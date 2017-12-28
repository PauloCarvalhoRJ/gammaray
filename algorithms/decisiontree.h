#ifndef DECISIONTREE_H
#define DECISIONTREE_H

#include <vector>

class DataValue;

/** The base class for the decision tree types used by algorithms. */
class DecisionTree
{
public:
    DecisionTree();
    virtual ~DecisionTree();

    /** Uses the underlying decision tree as a classifier to a given output data row, referenced by its row number.
     * This is just a front-end for the actual recursive classification function (see the protected section).
     * @param rowIdOutput Row number of output data to classify.
     * @param dependentVariableColumnID  The column id in the training data of the variable (supposedly categorical)
     *                                   to be predicted.
     * @param result A list with pair(s): the predicted value; how many times the value was found in training data.
     */
    virtual void classify( long rowIdOutput,
                           int dependentVariableColumnID,
                           std::vector< std::pair<DataValue, long> >& result) const = 0;

    /** Uses the underlying decision tree as a regression to a given output data row, referenced by its row number.
     * This is just a front-end for the actual recursive regression function (see the protected section).
     * @param rowIdOutput Row number of output data to produce an estimation.
     * @param dependentVariableColumnID  The column id in the training data of the variable (supposedly continuous)
     *                                   to be predicted.
     * @param mean The estimated result.
     * @param percent The percentage of the training data rows that was used in the regression decision.  This value is
     *                a measure of representativeness of the returned mean.
     */
    virtual void regress( long rowIdOutput,
                         int dependentVariableColumnID,
                         DataValue &mean,
                         double &percent ) const = 0;
};

#endif // DECISIONTREE_H
