#include "cartleafnode.h"
#include "../ialgorithmdatasource.h"
#include <vector>
#include <numeric>
#include <algorithm>

CARTLeafNode::CARTLeafNode(const IAlgorithmDataSource &trainingDataSource,
                           const std::vector<long> &rowIDs) : CARTNode(),
    m_trainingDataSource( trainingDataSource )
{
    // The = operator of std::vector copies elements from a container to the other.
    m_rowIndexes = rowIDs;
}

void CARTLeafNode::getUniqueTrainingValuesWithCounts(int columnID,
                                                     std::vector<std::pair<DataValue, long> > &result)
{
    //fetch and sort the values
    std::vector<DataValue> values;
    values.reserve( m_rowIndexes.size() );
    std::vector<long>::const_iterator it = m_rowIndexes.begin();
    for( ; it != m_rowIndexes.cend(); ++it )
        values.push_back( m_trainingDataSource.getDataValue( *it, columnID ) );
    std::sort( values.begin(), values.end() );

    //mount the output with counts.
    result.reserve( values.size() );
    for(std::vector<DataValue>::iterator it = values.begin(); it != values.end(); ++it){
        if( result.empty() || !(result.back().first == *it) ) //reuse the == operator of DataValue
            result.emplace_back( *it, 1 );
        else
            result.back().second++;
    }
}

double operator+( double v1, DataValue v2 ){
    return v2 + v1;
}

void CARTLeafNode::getMeanOfTrainingValuesWithPercentage( int columnID, DataValue &mean, double &percentage )
{
    //creates a vector with the values referenced by this node.
    std::vector<DataValue> values;
    std::vector<long>::const_iterator it = m_rowIndexes.begin();
    values.reserve( m_rowIndexes.size() );
    for( ; it != m_rowIndexes.cend(); ++it )
        values.push_back( m_trainingDataSource.getDataValue( *it, columnID ) );

    //return the percentage of training data rows referred by this leaf node with respect to the whole training set.
    percentage = values.size() / (double)m_trainingDataSource.getRowCount();

    //return the mean of values referred by this leaf nodes.
    mean = std::accumulate( values.begin(), values.end(), 0.0 ) / values.size();
}

