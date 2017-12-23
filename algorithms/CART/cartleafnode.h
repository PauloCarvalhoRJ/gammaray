#ifndef CARTLEAFNODE_H
#define CARTLEAFNODE_H

#include "cartnode.h"

#include <list>

class IAlgorithmDataSource;
class DataValue;

/** The CARTLeafNode is a leaf node of a CART (classification and regression decision tree). */
class CARTLeafNode : public CARTNode
{
public:
    CARTLeafNode( const IAlgorithmDataSource& trainingDataSource,
                  const std::list<long> &rowIDs );

    /** Fills the passed list with pairs of unique training data values with their counts found in the given column.
     *  Only the rows refered to in m_rowIndexes are considered.  Any previous contents in result list are erased.
     */
    void getUniqueTrainingValuesWithCounts( int columnID,
                                            std::list<std::pair<DataValue, long> > &result );

    /** Returns the mean and proportion of rows in the training set used in the mean of values refered by this node.
     *  Only the rows refered to in m_rowIndexes are considered in the mean.
     */
    void getMeanOfTrainingValuesWithPercentage( int columnID,
                                                DataValue &mean,
                                                double& percentage );

    //CARTNode interface
    virtual bool isLeaf() const { return true; }
protected:
    /** The reference to the training data source. */
    const IAlgorithmDataSource& m_trainingDataSource;

    /** The list of training data row indexes referenced by this node. The root node references all data rows. */
    std::list<long> m_rowIndexes;
};

#endif // CARTLEAFNODE_H
