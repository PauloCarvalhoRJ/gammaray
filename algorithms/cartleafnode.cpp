#include "cartleafnode.h"

CARTLeafNode::CARTLeafNode(const std::list<long> &rowIDs) : CARTNode()
{
    // The = operator of std::list copies elements from a container to the other.
    m_rowIndexes = rowIDs;
}

