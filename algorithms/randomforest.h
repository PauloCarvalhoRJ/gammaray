#ifndef RANDOMFOREST_H
#define RANDOMFOREST_H

#include <vector>
#include <list>

class IAlgorithmDataSource;
class CART;
class DataValue;

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
                 unsigned int B,
                 long seed);

    virtual ~RandomForest();

    /** Uses the underlying CART decision trees as classifiers to a given output data row, referenced by its row number.
     * @param rowIdOutput Row number of output data to classify.
     * @param dependentVariableColumnID  The column id in the training data of the variable to be predicted.
     * @param result A pair: the predicted value (by majority vote); 1.0 - the ratio between the number of the most voted class
     *                       and the total number of votes, which is a mesure of uncertainty.
     */
    void classify(long rowIdOutput,
                  int dependentVariableColumnID,
                  std::pair<DataValue, double> &result) const;
protected:

    /** The data to be bagged and used to build the decision trees. */
    const IAlgorithmDataSource& m_trainingData;

    /** The data to be classified. */
    IAlgorithmDataSource& m_outputData;

    /** The decision trees. */
    std::vector< CART* > m_trees;

    /** The number of trees. */
    unsigned int m_B;

    /** Repository of the data sources created internally so they are garbage collected in the destructor. */
    std::vector<IAlgorithmDataSource*> m_tmpDataSources;

};

#endif // RANDOMFOREST_H
