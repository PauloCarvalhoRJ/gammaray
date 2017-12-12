#ifndef CART_H
#define CART_H

#include "cartnode.h"
#include "cartsplitcriterion.h"

#include <list>

class CARTNode;
class IAlgorithmDataSource;

/** The CART class represents the CART algorithm, which serves to build decision trees from data to classify or
 * to perform regressions.  CART stands for Classification and Regression Tree.
 */
class CART
{
public:

    CART( const IAlgorithmDataSource& data );

protected:

    /** The root of the CART tree. */
    CARTNode m_root;

    /** The data from which the CART tree is built. */
    const IAlgorithmDataSource& m_data;

    /**
     * Performs data split for the CART algorithm.
     * @param rowIDs The set of row id's to be split.
     * @param criterion The split criterion.
     * @param trueSideRowIDs The set of ids of rows that match the criterion.
     * @param falseSideRowIDs The set of ids of rows that don't match the criterion.
     */
    void split( const std::list<int>& rowIDs,
                const CARTSplitCriterion& criterion,
                std::list<int>& trueSideRowIDs,
                std::list<int>& falseSideRowIDs );

    /** Returns a collection of the unique values found in the given column of a set of
     * rows (given by a list of row numbers).  The passed list is reset. */
    void getUniqueDataValues( std::list<DataValue>& result,
                              const std::list<long> &rowIDs,
                              int columnIndex ) const;

    /** Counts the categorical values found in the given column.  Unspecified behavior ensues if you count
     *  classes in calumns holding continuous values.  This method resets the passed list.
     * @param result A list of pairs: the first value is a categorical value that was counted.  The second
     *               value will hold the count.
     * @param rowIDs A list of row numbers from wich to take the counts.
     * @param columnIndex The index of a column holding categorical values.  See isContinuous().
     */
    void getCategoriesCounts(std::list< std::pair< DataValue, long > >& result,
                              const std::list<long> &rowIDs,
                              int columnIndex) const;

    /**
     * Computes the Gini impurity factor for a given variable in a set of rows.
     * This factor is a likelyhood of being incorrect in picking a category.  A factor of zero means you will
     * always pick the correct class (all class values in the column are the same).
     */
    double giniImpurity( const std::list<int>& rowIDs );
};

#endif // CART_H
