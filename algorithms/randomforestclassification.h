#ifndef RANDOMFORESTCLASSIFICATION_H
#define RANDOMFORESTCLASSIFICATION_H

#include <vector>

class IAlgorithmDataSource;
class CART;

/**
 * The RandomForestClassification class represents a random forest algorithm for classification purposes, that is, the
 * predicted variable is categorical (e.g.: lithofacies).  Briefly, it bootstraps the data samples (input)
 * B times and fits a decision tree to each one of the new sample set.  Then, the algorithm averages the B trees
 * to get the final classification or regression decision tree.  The resulting tree is less overfitted and the
 * yielded classification/regression shows less variance than obtained by applying one tree fitted to the original
 * data.
 */
class RandomForestClassification
{
public:

    /**
     * The constructor creates CART decision trees from radomly generated sample sets from the original set (bagging).
     *
     * @param B The number of trees.  Low values mean faster computation but more overfitting.  Higher values mean
     *          a smoother classification/regression but more misses.
     */
    RandomForestClassification( const IAlgorithmDataSource& trainingData,
                                const IAlgorithmDataSource& outputData,
                                unsigned int B );

protected:

    /** The data to be bagged and used to build the decision trees. */
    const IAlgorithmDataSource& m_trainingData;

    /** The data to be classified. */
    const IAlgorithmDataSource& m_outputData;

    /** The decision trees. */
    std::vector< CART > m_trees;

    /** The number of trees. */
    unsigned int m_B;

};

#endif // RANDOMFORESTCLASSIFICATION_H
