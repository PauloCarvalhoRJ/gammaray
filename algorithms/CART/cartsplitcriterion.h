#ifndef CARTSPLITCRITERION_H
#define CARTSPLITCRITERION_H

#include "../ialgorithmdatasource.h"

/** The CART split criterion is used to define CART (a decision tree) branching durin CART algorithm run. */
class CARTSplitCriterion
{
public:

    /**
     * @param trainingData Reference to a set of training data rows.
     * @param outputData Reference to a set of data rows that will be classified or estimated.
     * @param columnNumber The column index corresponding to the variable of this split criterion.
     * @param dataValue The data value that defines this split criterion.
     */
    CARTSplitCriterion( const IAlgorithmDataSource& trainingData,
                        const IAlgorithmDataSource& outputData,
                        int columnNumber,
                        DataValue dataValue );

    /** The attribution operator. */
    CARTSplitCriterion operator =( const CARTSplitCriterion& original );

    /** Tests whether the training data row given its index matches the split criterion.
     *  If true, the CART algorith will split the data set at the given row, assigning
     *  each partition to two new CARTNode objects to be added to a CART tree.
     * This function is tipically called by tree-building algorithms.
     */
    bool trainingMatches( long rowIndexTraining  ) const;

    /** Tests whether the output data row given its index matches the split criterion.
     * This function is tipically called by classification and regression algorithms.
     */
    bool outputMatches( long rowIndexOutput ) const;

protected:
    const IAlgorithmDataSource& m_trainingData;
    const IAlgorithmDataSource& m_outputData;
    int m_columnNumber;
    DataValue m_criterionValue;
};

#endif // CARTSPLITCRITERION_H
