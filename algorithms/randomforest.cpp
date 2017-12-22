#include "randomforest.h"
#include "CART/cart.h"
#include "bootstrap.h"
#include "ialgorithmdatasource.h"
#include <limits>

/** A temporary data source for Random Forest run. */
class TmpDataSource : public IAlgorithmDataSource{
    // IAlgorithmDataSource interface
public:

    TmpDataSource() : IAlgorithmDataSource(),
        m_data()
    {}

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
    Bootstrap bagger( trainingData, ResamplingType::CASE, seed );

    TmpDataSource baggedData;

    //For the wanted number of trees.
    for( unsigned int iTree = 0; iTree < m_B; ++iTree ){

        //bagg the training set
        bagger.resample( baggedData, trainingData.getRowCount() );

        //Create a CART decision tree for it
        m_trees.push_back( CART( trainingData, outputData, trainingFeatureIDs, outputFeatureIDs  ) );
    }
}
