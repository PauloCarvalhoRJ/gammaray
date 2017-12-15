#ifndef CARTDECISIONNODE_H
#define CARTDECISIONNODE_H

#include "cartnode.h"
#include "cartsplitcriterion.h"

/** The CARTDecisionNode is a non-leaf node of a CART (classification and regression decision tree). */
class CARTDecisionNode : public CARTNode
{
public:
    CARTDecisionNode(const CARTSplitCriterion& splitCriterion,
                           CARTNode* trueSideChildNode,
                           CARTNode* falseSideChildNode,
                     const IAlgorithmDataSource& outputDataSource);

    /** Tests whether the refered data row in the output data set satisfies the split criterion of this decision node. */
    bool criterionMatches( long rowIdOutput );

    //CARTNode interface
    virtual bool isLeaf() const { return false; }
protected:
    CARTSplitCriterion m_splitCriterion;
    const IAlgorithmDataSource& m_outputDataSource;
};

#endif // CARTDECISIONNODE_H
