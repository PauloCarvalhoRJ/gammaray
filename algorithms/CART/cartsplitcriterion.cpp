#include "cartsplitcriterion.h"

CARTSplitCriterion::CARTSplitCriterion(const IAlgorithmDataSource &trainingData,
                                       const IAlgorithmDataSource &outputData,
                                       int columnNumber,
                                       DataValue dataValue) :
    m_trainingData( trainingData ),
    m_outputData( outputData ),
    m_columnNumber( columnNumber ),
    m_criterionValue( dataValue )
{
}

CARTSplitCriterion& CARTSplitCriterion::operator =(const CARTSplitCriterion &original)
{
    //these are const, assumes the training and output data is the same for all criteria, so, be careful to
    //not mix criteria used in different classification/regression runs.
    //m_trainingData = original.m_trainingData;
    //m_outputData =  original.m_outputData;
    //  TODO: add some kind of assert to enforce equality of the data sources in the assignment operator.
    m_columnNumber = original.m_columnNumber;
    m_criterionValue = original.m_criterionValue;
    return *this;
}

bool CARTSplitCriterion::trainingMatches(long rowIndexTraining ) const
{
    //get the value for the split criterion.
    DataValue criterionValue = m_trainingData.getDataValue( rowIndexTraining, m_columnNumber );

    //if the data value is categorical, then the criterion is being equal to the criterion value.
    if( criterionValue.isCategorical() )
        return criterionValue == m_criterionValue;
    //if the data value is continuous, then the criterion is being greater than or equal to the criterion value.
    else
        return !(criterionValue < m_criterionValue); // !< is the same as >= (reuse the < operator of DataValue)
}

bool CARTSplitCriterion::outputMatches(long rowIndexOutput) const
{
    //get the value for the split criterion.
    DataValue criterionValue = m_outputData.getDataValue( rowIndexOutput, m_columnNumber );

    //if the data value is categorical, then the criterion is being equal to the criterion value.
    if( criterionValue.isCategorical() )
        return criterionValue == m_criterionValue;
    //if the data value is continuous, then the criterion is being greater than or equal to the criterion value.
    else
        return !(criterionValue < m_criterionValue); // !< is the same as >= (reuse the < operator of DataValue)
}
