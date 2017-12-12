#ifndef CARTNODE_H
#define CARTNODE_H

#include <list>

/** The CARTNode is a node of a CART (classification and regression decision tree). */
class CARTNode
{
public:
    CARTNode();

    void addRowIndex( long rowIndex );

protected:
    /** The list of data row indexes referenced by this node. The root node references all data rows. */
    std::list<long> m_rowIndexes;
};

#endif // CARTNODE_H
