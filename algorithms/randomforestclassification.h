#ifndef RANDOMFOREST_H
#define RANDOMFOREST_H

class IAlgorithmDataSource;

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
     * @param B The number of trees.  Low values mean faster computation but more overfitting.  Higher values mean
     *          a smoother classification/regression but more misses.
     */
    RandomForestClassification( const IAlgorithmDataSource& data, unsigned int B );

};

#endif // RANDOMFOREST_H
