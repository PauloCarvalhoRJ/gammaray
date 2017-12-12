#include "cartnode.h"

CARTNode::CARTNode()
{
}

void CARTNode::addRowIndex(long rowIndex)
{
    m_rowIndexes.push_back( rowIndex );
}
