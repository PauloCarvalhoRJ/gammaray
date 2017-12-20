#ifndef CARTSPLITCRITERION_H
#define CARTSPLITCRITERION_H

#include "../ialgorithmdatasource.h"
#include <map>

/** The CART split criterion is used to define CART (a decision tree) branching durin CART algorithm run. */
class CARTSplitCriterion
{
public:

    /**
     * @param trainingData Reference to a set of training data rows.
     * @param outputData Reference to a set of data rows that will be classified or estimated.
     * @param columnNumber The column index (in the training data set) corresponding to the variable of this split criterion.
     * @param dataValue The data value that defines this split criterion.
     * @param a map object mapping feature column numbers in the training data set to feature column numbers
     *        in the output data set.  This is necessary because rarely a same feature has the same column number
     *        in both datasets.
     */
    CARTSplitCriterion( const IAlgorithmDataSource& trainingData,
                        const IAlgorithmDataSource& outputData,
                        int columnNumberTrainingData,
                        DataValue dataValue,
                        const std::map<int,int>& training2outputFeatureIndexesMap );

    /** The attribution operator.
     *  @note Since the references to the data sources are const, only columnNumber criterion value are assigned.
     *        So, be careful to not mix criteria used in different classification/regression runs with different data sets.
     */
    CARTSplitCriterion &operator =( const CARTSplitCriterion& original );

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
    int m_columnNumberTrainingData;
    DataValue m_criterionValue;
    const std::map<int,int>& m_training2outputFeatureIndexesMap;
};

#endif // CARTSPLITCRITERION_H
