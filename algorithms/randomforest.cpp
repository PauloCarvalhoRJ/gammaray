#include "randomforest.h"
#include "CART/cart.h"
#include "bootstrap.h"
#include "ialgorithmdatasource.h"
#include <limits>

/** A temporary data source for Random Forest run. */
class TmpDataSource : public IAlgorithmDataSource{
    // IAlgorithmDataSource interface
public:

    virtual long getRowCount() const {
        return m_data.size();
    }
    virtual int getColumnCount() const {
        if( isEmpty() )
            return 0;
        return m_data[0].size();
    }
    virtual void clear() {
        m_data.clear();
    }
    virtual void reserve(long rowCount, int columnCount) {
        m_data.assign( rowCount, std::vector<DataValue> ( columnCount, DataValue(std::numeric_limits<double>::quiet_NaN()) ) );
    }
    virtual void setDataValue(long rowIndex, int columnIndex, DataValue value) {
        m_data[rowIndex][columnIndex] = value;
    }
    virtual DataValue getDataValue(long rowIndex, int columnIndex) const {
        return m_data[rowIndex][columnIndex];
    }
protected:
    std::vector< std::vector< DataValue > > m_data;
};


RandomForest::RandomForest(const IAlgorithmDataSource &trainingData,
                                                       IAlgorithmDataSource &outputData,
                                                       const std::list<int> &trainingFeatureIDs,
                                                       const std::list<int> &outputFeatureIDs,
                                                       unsigned int B,
                                                       long seed ) :
    m_trainingData( trainingData ),
    m_outputData( outputData ),
    m_B( B )
{
    //Create an object to create training subsamples
    Bootstrap bagger( trainingData, ResamplingType::CASE, seed );

    //For the wanted number of trees.
    for( unsigned int iTree = 0; iTree < m_B; ++iTree ){

        //Create a temporary data storage for the bagged training data
        TmpDataSource* baggedTrainingData = new TmpDataSource();
        m_tmpDataSources.push_back( baggedTrainingData );

        //bagg the training set
        bagger.resample( *baggedTrainingData, trainingData.getRowCount() );

        //Create a CART decision tree for the bagged training data
        m_trees.push_back( new CART( *baggedTrainingData, outputData, trainingFeatureIDs, outputFeatureIDs  ) );
    }
}

RandomForest::~RandomForest()
{
    //GC the objects created in the constructor.
    while( ! m_trees.empty() ){
        delete m_trees.back();
        m_trees.pop_back();
    }
    while( ! m_tmpDataSources.empty() ){
        delete m_tmpDataSources.back();
        m_tmpDataSources.pop_back();
    }
}

void RandomForest::classify(long rowIdOutput,
                            int dependentVariableColumnID,
                            std::pair<DataValue, double> &result) const
{
    //a list with the possibly different classes found
    std::list<int> classesFound;

    //for each decision tree.
    std::vector< CART* >::const_iterator itTree = m_trees.cbegin();
    for(; itTree != m_trees.cend(); ++itTree ){

        //get the decision tree
        CART *CARTtree = *itTree;

        //classify the data using one decision tree
        std::list< std::pair< DataValue, long> > localResult;
        CARTtree->classify( rowIdOutput, dependentVariableColumnID, localResult );

        //get the classification result (one vote) from the decision tree
        std::list< std::pair< DataValue, long> >::iterator it = localResult.begin();
        for( ; it != localResult.end(); ++it ){
            classesFound.push_back( (*it).first.getCategorical() );
            break; //TODO: this causes only the first class value to be considerd
                   //      other values may come with different counts (assign uncertainty)
        }
    }

    //sort the votes
    classesFound.sort();

    //determine which class was most voted
    //TODO: ties are currently ignored, it simply sticks with the first most voted
    int mostVotedClass = -999999999;
    long mostVotedClassCount = 0;
    long currentCount = 0;
    int previousClass = -999999999; //init with some unlikely class id
    std::list<int>::iterator itClasses = classesFound.begin();
    for( ; itClasses != classesFound.end(); ++itClasses ){
        int currentClass = *itClasses;
        if( currentClass != previousClass )
            currentCount = 0;
        ++currentCount;
        if( currentCount > mostVotedClassCount ){
            mostVotedClassCount = currentCount;
            mostVotedClass = currentClass;
        }
        previousClass = currentClass;
    }

    //compute the voting uncertainty
    double uncertainty = 1.0 - mostVotedClassCount / (double)classesFound.size();

    //"return" the result
    result.first = DataValue( mostVotedClass );
    result.second = uncertainty;
}
