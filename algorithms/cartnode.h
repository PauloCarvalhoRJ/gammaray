#ifndef CARTNODE_H
#define CARTNODE_H

#include <memory>

/** The generic CART tree node object. */
class CARTNode
{
public:
    CARTNode();
protected:
    std::unique_ptr< CARTNode > m_trueSideChildNode;
    std::unique_ptr< CARTNode > m_falseSideChildNode;
};

#endif // CARTNODE_H
