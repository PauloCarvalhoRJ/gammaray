#ifndef SVDFACTORTREE_H
#define SVDFACTORTREE_H

#include <vector>
#include "svdfactor.h"

/**
 * @brief The SVDFactorTree class is a collection of an SVD decomposition hierarchy (SVD factors can be decomposed into
 * SVD factors themselves).
 */
class SVDFactorTree
{
public:
    SVDFactorTree();

    void addFactor( SVDFactor&& factor );

private:
    std::vector< SVDFactor > m_rootFactors;
};

#endif // SVDFACTORTREE_H
