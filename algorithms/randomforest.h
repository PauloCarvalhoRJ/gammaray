#ifndef RANDOMFOREST_H
#define RANDOMFOREST_H

#include <vector>
#include <list>
#include "bootstrap.h"

class IAlgorithmDataSource;
class DecisionTree;
class DataValue;

/*! The tree type used to build the forest.  It also refers to the algorithm used to build the trees from training data. */
enum class TreeType : unsigned int{
    CART /*! CART (Classification and Regression Tree) trees are used. */
    //ID3 /*! ID3 (Iterative Dichotomiser 3) trees are used.  ID3 is used in classification. */
    //C4_5 /*! C4.5 trees are used. C4.5 is a development of ID3 and is used in classification. */
    //C5_0 /*! C5.0 trees are used. C5.0 is a faster and low memory development of C4.5. */
    //CHAID /*! CHAID (Chi-squared Automatic Interaction Detector) trees are used. */
    //MARS /*! MARS trees are used.  MARS trees are designed to be optimal with numeric data. */
    //CIT /*! Conditional Inference Trees are used.  */
};

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
     * The constructor creates decision trees from radomly generated sample sets from the original set (bagging).
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
                 long seed,
                 ResamplingType bootstrap,
                 TreeType treeType);

    virtual ~RandomForest();

    /** Uses the underlying decision trees as classifiers to a given output data row, referenced by its row number.
     * @param rowIdOutput Row number of output data to classify.
     * @param dependentVariableColumnID  The column id in the training data of the variable to be predicted.
     * @param result A pair: the predicted value (by majority vote); 1.0 - the ratio between the number of the most voted class
     *                       and the total number of votes, which is a mesure of uncertainty.
     */
    void classify(long rowIdOutput,
                  int dependentVariableColumnID,
                  std::pair<DataValue, double> &result) const;

    /** Uses the underlying decision trees as regressions to a given output data row, referenced by its row number.
     * @param rowIdOutput Row number of output data to estimate.
     * @param dependentVariableColumnID  The column id in the training data of the variable to be predicted.
     * @param mean The regression value.
     * @param variance The variance between the individual estimates given by each decision tree.
     */
    void regress( long rowIdOutput,
                  int dependentVariableColumnID,
                  DataValue& mean,
                  DataValue& variance ) const;

protected:

    /** The data to be bagged and used to build the decision trees. */
    const IAlgorithmDataSource& m_trainingData;

    /** The data to be classified. */
    IAlgorithmDataSource& m_outputData;

    /** The decision trees. */
    std::vector< DecisionTree* > m_trees;

    /** The number of trees. */
    unsigned int m_B;

    /** Repository of the data sources created internally so they are garbage collected in the destructor. */
    std::vector<IAlgorithmDataSource*> m_tmpDataSources;

};

#endif // RANDOMFOREST_H
