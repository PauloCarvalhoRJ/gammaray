#include "cartdecisionnode.h"

CARTDecisionNode::CARTDecisionNode(const CARTSplitCriterion &splitCriterion,
                                         CARTNode *trueSideChildNode,
                                         CARTNode *falseSideChildNode) : CARTNode(),
    m_splitCriterion( splitCriterion )
{
    m_trueSideChildNode.reset( trueSideChildNode );
    m_falseSideChildNode.reset( falseSideChildNode );
}
