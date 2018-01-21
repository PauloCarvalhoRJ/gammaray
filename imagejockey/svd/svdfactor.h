#ifndef SVDFACTOR_H
#define SVDFACTOR_H

#include "spectral/spectral.h"

//third-party library Eigen
namespace spectral{
   class array;
}

/**
 * @brief The SVDFactor class represents one factor obtained from Singular Value Decomposition (SVD).
 */
class SVDFactor
{
public:
    /**
     * @param parentFactor If set, this SVD factor is a decomposition of another SVD factor.
     */
    SVDFactor( spectral::array&& factorData, SVDFactor* parentFactor = nullptr );
private:
    SVDFactor* m_parentFactor;
    spectral::array m_factorData;
};

#endif // SVDFACTOR_H
