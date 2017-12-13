#ifndef CARTSPLITCRITERION_H
#define CARTSPLITCRITERION_H

#include "ialgorithmdatasource.h"

/** The CART split criterion is used to define CART (a decision tree) branching durin CART algorithm run. */
class CARTSplitCriterion
{
public:

    /**
     * @param data Reference to a set of data rows.
     * @param columnNumber The column index corresponding to the variable of this split criterion.
     * @param dataValue The data value that defines this split criterion.
     */
    CARTSplitCriterion( const IAlgorithmDataSource& data, int columnNumber, DataValue dataValue );

    /** The attribution operator. */
    CARTSplitCriterion operator =( const CARTSplitCriterion& original );

    /** Tests whether the data row given its index matches the split criterion.
     *  If true, the CART algorith will split the data set at the given row, assigning
     *  each partition to two new CARTNode objects to be added to a CART tree.
     */
    bool matches( long rowIndex  ) const;

protected:
    const IAlgorithmDataSource& m_data;
    int m_columnNumber;
    DataValue m_criterionValue;
};

#endif // CARTSPLITCRITERION_H
