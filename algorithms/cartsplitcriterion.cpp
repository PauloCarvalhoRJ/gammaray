#include "cartsplitcriterion.h"

CARTSplitCriterion::CARTSplitCriterion(const IAlgorithmDataSource &data, int columnNumber, DataValue dataValue) :
    m_data( data ),
    m_columnNumber( columnNumber ),
    m_criterionValue( dataValue )
{
}

CARTSplitCriterion CARTSplitCriterion::operator =(const CARTSplitCriterion &original)
{
    return CARTSplitCriterion( original.m_data, original.m_columnNumber, original.m_criterionValue );
}

bool CARTSplitCriterion::matches( long rowIndex ) const
{
    //get the value for the split criterion.
    DataValue criterionValue = m_data.getDataValue( rowIndex, m_columnNumber );

    //if the data value is categorical, then the criterion is being equal to the criterion value.
    if( criterionValue.isCategorical() )
        return criterionValue == m_criterionValue;
    //if the data value is continuous, then the criterion is being greater than or equal to the criterion value.
    else
        return !(criterionValue < m_criterionValue); // !< is the same as >= (reuse the < operator of DataValue)
}
