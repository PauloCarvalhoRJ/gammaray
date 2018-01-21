#include "svdfactor.h"

SVDFactor::SVDFactor(spectral::array &&factorData, SVDFactor *parentFactor) :
    m_parentFactor( parentFactor ),
    m_factorData( std::move( factorData) )
{
}
