#ifndef RANDOMFOREST_H
#define RANDOMFOREST_H

#include <vector>
#include <list>

class IAlgorithmDataSource;
class CART;

/**
 * The RandomForest class represents a random forest algorithm for classification or regression purposes, that is, the
 * predicted variable is categorical (e.g.: lithofacies).  Briefly, it bootstraps the data samples (input)
 * B times and fits a decision tree to each one of the new sample set.  Then, the algorithm averages the B trees
 * to get the final classification or regression decision tree.  The resulting tree is less overfitted and the
 * yielded classification/regression shows less variance than obtained by applying one tree fitted to the original
 * data.
 */
class RandomForest
{
public:

    /**
     * The constructor creates CART decision trees from radomly generated sample sets from the original set (bagging).
     *
     * @param B The number of trees.  Low values mean faster computation but more overfitting.  Higher values mean
     *          a smoother classification/regression but more misses.
     * @param seed The seed for the random number generator.
     */
    RandomForest(const IAlgorithmDataSource& trainingData,
                               IAlgorithmDataSource &outputData,
                               const std::list<int> &trainingFeatureIDs,
                               const std::list<int> &outputFeatureIDs,
                               unsigned int B ,
                               long seed);

protected:

    /** The data to be bagged and used to build the decision trees. */
    const IAlgorithmDataSource& m_trainingData;

    /** The data to be classified. */
    IAlgorithmDataSource& m_outputData;

    /** The decision trees. */
    std::vector< CART > m_trees;

    /** The number of trees. */
    unsigned int m_B;

};

#endif // RANDOMFOREST_H
