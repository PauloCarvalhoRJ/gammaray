#ifndef CARTLEAFNODE_H
#define CARTLEAFNODE_H

#include "cartnode.h"

#include <list>

/** The CARTLeafNode is a leaf node of a CART (classification and regression decision tree). */
class CARTLeafNode : public CARTNode
{
public:
    CARTLeafNode( const std::list<long> &rowIDs );

protected:
    /** The list of data row indexes referenced by this node. The root node references all data rows. */
    std::list<long> m_rowIndexes;
};

#endif // CARTLEAFNODE_H
