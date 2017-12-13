#include "cartleafnode.h"

CARTLeafNode::CARTLeafNode()
{
}

void CARTLeafNode::addRowIndex(long rowIndex)
{
    m_rowIndexes.push_back( rowIndex );
}
