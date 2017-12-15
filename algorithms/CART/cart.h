#ifndef CART_H
#define CART_H

#include "cartleafnode.h"
#include "cartsplitcriterion.h"

#include <list>
#include <memory>

class CARTLeafNode;
class IAlgorithmDataSource;

/** The CART class represents the CART algorithm, which serves to build decision trees from data to classify or
 * to perform regressions.  CART stands for Classification and Regression Tree.
 */
class CART
{
public:

    /** Builds a CART tree using the given data set and passing a list of column IDs
     * corresponding to the features/variables to use as training data.  The root of
     * the resulting CART tree is pointed by the m_root member.
     */
    CART( const IAlgorithmDataSource& trainingData,
          IAlgorithmDataSource& outputData,
          const std::list<int> &featureIDs );

    /** Uses the underlying CART decision tree as a classifier to a given output data row, referenced by its row number.
     * This is just a front-end for the actual recursive classification function (see the protected section).
     * @param rowIdOutput Row number of output data to classify.
     * @param dependentVariableColumnID  The column id in the training data of the variable to be predicted.
     * @param result A list with pair(s): the predicted value; how many times the value was found in training data.
     */
    void classify( long rowIdOutput,
                   int dependentVariableColumnID,
                   std::list< std::pair<DataValue, long> >& result) const;

protected:

    /** The root of the CART tree. */
    std::unique_ptr<CARTNode> m_root;

    /** The data from which the CART tree is built. */
    const IAlgorithmDataSource& m_trainingData;

    /** The data to receive predictions. */
    IAlgorithmDataSource& m_outputData;

    /**
     * Performs data split for the CART algorithm.  The output lists are cleared before splitting.
     * @param rowIDs The set of row id's to be split.
     * @param criterion The split criterion.
     * @param trueSideRowIDs The set of ids of rows that match the criterion.
     * @param falseSideRowIDs The set of ids of rows that don't match the criterion.
     */
    void split(const std::list<long> &rowIDs,
               const CARTSplitCriterion &criterion,
               std::list<long> &trueSideRowIDs,
               std::list<long> &falseSideRowIDs ) const;

    /** Returns a collection of the unique values found in the given column of a set of
     * rows (given by a list of row numbers).  The passed list is reset. */
    void getUniqueDataValues( std::list<DataValue>& result,
                              const std::list<long> &rowIDs,
                              int columnIndex ) const;

    /** Counts the categorical values found in the given column.  Unspecified behavior ensues if you count
     *  classes in columns holding continuous values.  This method resets the passed list.
     * @param result A list of pairs: the first value is a categorical value that was counted.  The second
     *               value will hold the count.
     * @param rowIDs A list of row numbers from wich to take the counts.
     * @param columnIndex The index of a column holding categorical values.  See isContinuous().
     */
    void getCategoriesCounts( std::list< std::pair< DataValue, long > >& result,
                              const std::list<long> &rowIDs,
                              int columnIndexWithCategoricalValues) const;

    /**
     * Computes the Gini impurity factor for a given variable in a set of rows.
     * This factor is a likelyhood of being incorrect in picking a category.  A factor of zero means you will
     * always pick the correct class (all class values in the column are the same).
     */
    double getGiniImpurity(const std::list<long> &rowIDs,
                           int columnIndex ) const ;

    /**
     * Calculates the information gain given a data split and the ammout of uncertainty before the split.
     * If the returned value is positive, than the proposed split will decrease uncertainty.
     * @param rowIDsTrueSide Row IDs of data that matched the split criterion.
     * @param rowIDsFalseSide Rod IDs of data that did not match the split criterion.
     * @param columnIndex The column ID of the variable used in the split criterion.
     * @param impurityFactorBeforeTheSplit The uncertainty measure between 0.0 and 1.0 before the split.
     */
    double getSplitInformationGain( const std::list<long> &rowIDsTrueSide,
                                    const std::list<long> &rowIDsFalseSide,
                                    int columnIndex,
                                    double impurityFactorBeforeTheSplit ) const;

    /**
     * Returns the CART tree partition criterion with the highest information gain among the possible ones
     * that can be made with the data given by row numbers (IDs).  Information gain is defined by reduction
     * of uncertainty (sum or impurity) in the tree nodes below.  The goal is to get large data subsets with
     * low uncertainty until we get leaf nodes with pure (0% chance of incorrect picking) or at least with
     * low impurity.
     * @param rowIDs Row IDs of the row set.
     * @param featureIDs Column IDs of the variables/features participating in the training data.
     */
    std::pair<CARTSplitCriterion, double> getSplitCriterionWithMaximumInformationGain(const std::list<long> &rowIDs,
                                                                                      const std::list<int> &featureIDs) const;

    /** Builds a CART tree hierarchy from the given set of data rows (referenced by a list of row numbers).
     * @param rowIDs Row IDs of the row set.
     * @param featureIDs Column IDs of the variables/features participating in the training data.
     */
    CARTNode* makeCART( const std::list<long> &rowIDs , const std::list<int> &featureIDs ) const;

    /** The actual recursive implementation of classify().
     * @param decisionTreeNode the node of the tree holding the decision hierarchy to classify.  If nullptr, the root node is used.
     */
    void classify( long rowIdOutput,
                   int dependentVariableColumnID,
                   const CARTNode* decisionTreeNode,
                   std::list< std::pair<DataValue, long> >& result) const;
};

#endif // CART_H
