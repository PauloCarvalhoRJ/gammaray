#ifndef CARTNODE_H
#define CARTNODE_H

#include <memory>

/** The generic CART tree node object. */
class CARTNode
{
public:
    CARTNode();
    virtual bool isLeaf() const = 0;

    const CARTNode* getTrueSideChildNode() const { return m_trueSideChildNode.get(); }
    const CARTNode* getFalseSideChildNode() const { return m_falseSideChildNode.get(); }

protected:
    std::unique_ptr< CARTNode > m_trueSideChildNode;
    std::unique_ptr< CARTNode > m_falseSideChildNode;
};

#endif // CARTNODE_H
