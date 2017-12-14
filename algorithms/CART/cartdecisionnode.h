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
                           CARTNode* falseSideChildNode );
protected:
    CARTSplitCriterion m_splitCriterion;
};

#endif // CARTDECISIONNODE_H
