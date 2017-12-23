#include "cartleafnode.h"
#include "../ialgorithmdatasource.h"
#include <vector>
#include <numeric>

CARTLeafNode::CARTLeafNode(const IAlgorithmDataSource &trainingDataSource,
                           const std::list<long> &rowIDs) : CARTNode(),
    m_trainingDataSource( trainingDataSource )
{
    // The = operator of std::list copies elements from a container to the other.
    m_rowIndexes = rowIDs;
}

void CARTLeafNode::getUniqueTrainingValuesWithCounts(int columnID,
                                                     std::list<std::pair<DataValue, long> > &result)
{
    //creates a list with the unique values referenced by this node.
    std::list<DataValue> uniqueValues;
    std::list<long>::const_iterator it = m_rowIndexes.begin();
    for( ; it != m_rowIndexes.cend(); ++it )
        uniqueValues.push_back( m_trainingDataSource.getDataValue( *it, columnID ) );
    uniqueValues.sort();
    uniqueValues.unique();

    //mount the output with zero counts.
    result.clear();
    for(std::list<DataValue>::iterator it = uniqueValues.begin(); it != uniqueValues.end(); ++it){
        result.emplace_back( *it, 0 );
    }

    //for each of the unique values found.
    for( std::list<std::pair<DataValue, long> >::iterator it = result.begin(); it != result.end(); ++it){
        std::pair<DataValue, long>& pair = *it;
        //for each row
        for( std::list<long>::const_iterator rowIt = m_rowIndexes.cbegin(); rowIt != m_rowIndexes.cend(); ++rowIt ){
            if( m_trainingDataSource.getDataValue( *rowIt, columnID ) == pair.first ){
                pair.second++;
            }
        }
    }
}

double operator+( double v1, DataValue v2 ){
    return v2 + v1;
}

void CARTLeafNode::getMeanOfTrainingValuesWithPercentage( int columnID, DataValue &mean, double &percentage )
{
    //creates a vector with the values referenced by this node.
    std::vector<DataValue> values;
    std::list<long>::const_iterator it = m_rowIndexes.begin();
    for( ; it != m_rowIndexes.cend(); ++it )
        values.push_back( m_trainingDataSource.getDataValue( *it, columnID ) );

    //return the percentage of training data rows referred by this leaf node with respect to the whole training set.
    percentage = values.size() / (double)m_trainingDataSource.getRowCount();

    //return the mean of values referred by this leaf nodes.
    mean = std::accumulate( values.begin(), values.end(), 0.0 ) / values.size();
}

