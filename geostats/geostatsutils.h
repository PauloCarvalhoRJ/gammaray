#ifndef GEOSTATSUTILS_H
#define GEOSTATSUTILS_H

#include "matrix3x3.h"

/*! Variogram model types. */
enum class VariogramModel : unsigned int {
    SPHERICAL = 0 /*!< The spherical model. */
};

/**
 * The GeostatsUtils class contains static functions for geostatistics computations.
 */
class GeostatsUtils
{
public:
    GeostatsUtils();


    /** Returns the transform matrix corresponding to the given anisotropy ellipse.
     * The three angles follow the GSLib convention (though, angles are in radians).
     *  E.g. azimuth 0 means that the semi-major axis
     * points to North (y-axis aligned) and increases clockwise (geologist's convention).
     */
    static Matrix3X3<double> getAnisoTransform( double aSemiMajor, double aSemiMinor, double aSemiVert,
                                                double azimuth, double dip, double roll );

    /** Transforms the 3x1 vector-column (a1, a2, a3) with the given 3x3 matrix. */
    static void transform( Matrix3X3<double>& t, double& a1, double& a2, double& a3 );

    /** Returns a value for h (separation) given an anisotropy transform (obtained with getAnisoTranform() with
     * the aniso ellipsoid parameters) between two locations.  The h value is then entered into a variogram model
     *  (e.g. spheric) to get the covariance for the kriging equations.  This function transform the world coordinates
     * so to reduce the anisotropy to an isotropic case, the you can use getGama() to get the covariance value.
     */
    static double getH( double x0, double y0, double z0,
                        double x1, double y1, double z1,
                        Matrix3X3<double> &anisoTransform );

    /**
     * Returns the covariance accoring to the given model.
     * @param h separation argument (transformed with getH() if there is anisotropy).
     * @param range The a argument (the a value of the semi-major axis if there is anisotropy).
     * @param contribution Variance contribution (e.g. 20.0).
     */
    static double getGamma( VariogramModel model, double h, double range, double contribution );


};

#endif // GEOSTATSUTILS_H
