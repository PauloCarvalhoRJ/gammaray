#include "svdfactortree.h"

SVDFactorTree::SVDFactorTree()
{
}

void SVDFactorTree::addFactor(SVDFactor &&factor)
{
    m_rootFactors.push_back( std::move( factor ) );
}
