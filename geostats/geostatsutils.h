#ifndef GEOSTATSUTILS_H
#define GEOSTATSUTILS_H

#include "matrix3x3.h"
#include "matrixmxn.h"
#include "domain/variogrammodel.h"
#include <set>

class GridCell;
class SpatialLocation;

/**
 * The GeostatsUtils class contains static utilitary functions common to geostatistics algorithms.
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
     * Returns the covariance for a variogram structure.
     * @param h separation argument (transformed with getH() if there is anisotropy).
     * @param range The a argument (the a value of the semi-major axis if there is anisotropy).
     * @param contribution Variance contribution (e.g. 20.0).
     */
    static double getGamma( VariogramStructureType permissiveModel, double h, double range, double contribution );

    /**
     * Returns the total covariance according to a variogram model between two locations.
     * Includes the nugget effet contribution, if any.
     */
    static double getGamma(VariogramModel* model, SpatialLocation &locA, SpatialLocation &locB );

    /**
     * Creates a covariance matrix for the given set of samples.
     */
    static MatrixNXM<double> makeCovMatrix(std::multiset<GridCell>& samples,
                                           VariogramModel *variogramModel);

    /**
     * Creates a gamma matrix of the given set of samples against the estimation location cell.
     */
    static MatrixNXM<double> makeGammaMatrix(std::multiset<GridCell>& samples,
                                             GridCell& estimationLocation,
                                             VariogramModel *variogramModel);

    /**
     *  Returns a list of valued grid cells, ordered by topological proximity to the target cell.
     */
    static std::multiset<GridCell> getValuedNeighborsTopoOrdered(GridCell &cell,
                                                            int numberOfSamples,
                                                            int nColsAround,
                                                            int nRowsAround,
                                                            int nSlicesAround);
};

#endif // GEOSTATSUTILS_H
