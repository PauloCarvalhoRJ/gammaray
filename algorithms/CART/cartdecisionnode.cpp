#include "cartdecisionnode.h"

CARTDecisionNode::CARTDecisionNode(const CARTSplitCriterion &splitCriterion,
                                         CARTNode *trueSideChildNode,
                                         CARTNode *falseSideChildNode,
                                   const IAlgorithmDataSource &outputDataSource) : CARTNode(),
    m_splitCriterion( splitCriterion ),
    m_outputDataSource( outputDataSource )
{
    m_trueSideChildNode.reset( trueSideChildNode );
    m_falseSideChildNode.reset( falseSideChildNode );
}

bool CARTDecisionNode::criterionMatches(long rowIdOutput)
{
    return m_splitCriterion.outputMatches( rowIdOutput );
}
