#ifndef IJVARIOGRAPHICMODEL2D_H
#define IJVARIOGRAPHICMODEL2D_H

#include <vector>
#include "spectral/spectral.h"

class IJAbstractCartesianGrid;

/*! The allowed variogram models. */
enum class IJVariogramPermissiveModel : int {
    SPHERIC = 1,
    EXPONENTIAL,
    GAUSSIAN,
    POWER_LAW,
    COSINE_HOLE_EFFECT
};

/**
 * The IJVariographicStructure2D class models a single 2D variograaphic structure model.
 * Axis: the variographic range along the main axis of the anisotropy ellipsis
 * Ratio: the ratio between semi-minor axis and Axis.
 * Azimuth: the azimuth of Axis.
 * Contribution: the semi-variance contribution of the nested structure
 */
class IJVariographicStructure2D
{
public:
    IJVariographicStructure2D();
    IJVariographicStructure2D( double pRange,
                               double pRangeRatio,
                               double pAzimuth,
                               double pContribution );

    double range; //the variogram range along the main anisotropy axis
    double rangeRatio;
    double azimuth;
    double contribution;


    static int getNumberOfParameters();

    double getParameter( int index ) const;

    void setParameter( int index, double value );

    /** Adds to the passed grid, the values of the surface corresponding to this
     * variographic structur.  The values in each cell are ADDED to the computed
     * values, so make sure the values in the target grid are initialized appropriatelly.
     * @param invert If true, the semivariances start with contribution at the center and
     *               decreases with distance.  Set to false to get the normal variogram
     *               behavior, which increases with distance.
     */
    void addContributionToModelGrid( const IJAbstractCartesianGrid &gridWithGeometry,
                                     spectral::array& targetGrid,
                                     IJVariogramPermissiveModel model,
                                     bool invert ) const;
};

/**
 * The IJVariographicModel2D class models a full 2D variographic model.
 * It may have one or more variographic structures or even none (pure
 * nugget effect model).
 */
class IJVariographicModel2D
{
public:
    IJVariographicModel2D(double pNugget = 0.0 );

    double nugget;
    std::vector< IJVariographicStructure2D > structures;
};

#endif // IJVARIOGRAPHICMODEL2D_H
