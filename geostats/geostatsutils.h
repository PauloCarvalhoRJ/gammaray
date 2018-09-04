#ifndef GEOSTATSUTILS_H
#define GEOSTATSUTILS_H

#include "matrix3x3.h"
#include "matrixmxn.h"
#include "domain/variogrammodel.h"
#include "geostats/datacell.h"
#include "geostats/gridcell.h"
#include <set>

class SpatialLocation;

/*! Kriging type. */
enum class KrigingType : unsigned {
    SK = 0, /*!< Simple kriging. */
    OK      /*!< Originary kriging. */
};


/**
 * The GeostatsUtils class contains static utilitary functions common to geostatistics algorithms.
 */
class GeostatsUtils
{
public:
    GeostatsUtils();


    /** Returns the transform matrix corresponding to the given anisotropy ellipse.
     * The three angles follow the GSLib convention (angles in degrees).
     * E.g.: azimuth 90.0 means that the semi-major axis
     * points to East (x-axis aligned) and increases clockwise (geologist's convention).
     * Positive dip angles are "nose up" and posivite roll angles are "bank to right", following
     * aircraft manuevers analogy.
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
    static double getH(double x0, double y0, double z0,
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
	static double getGamma(VariogramModel* model, const SpatialLocation& locA, const SpatialLocation& locB );

    /**
     * Creates a covariance matrix for the given set of samples.
     * @param kType Kriging type.  If SK, then the matrix has only the covariances between
     *        the samples.  If OK, the matrix has an extra row and column with 1.0s, except
     *        for the last element of both (the last element of matrix), with is zero.
	 * @param returnGamma If true, the elements are variogram values (increases with distance).  The
	 *        default (false) makes the elements be correlogram values (decreases with
	 *        distance).  The variogram sill value is ignored if this parameter is true.
     */
	static MatrixNXM<double> makeCovMatrix(DataCellPtrMultiset & samples,
										   VariogramModel *variogramModel,
										   double variogramSill,
										   KrigingType kType = KrigingType::SK,
										   bool returnGamma = false);

    /**
     * Creates a gamma matrix of the given set of samples against the estimation location cell.
     * @param kType Kriging type.  If SK, then the matrix has only the covariances between
     *        the samples and the estimation location.  If OK, the matrix has an extra element == 1.0.
	 * @param returnGamma If true, the elements are variogram values (increases with distance).  The
	 *        default (false) makes the elements be correlogram values (decreases with
	 *        distance).  The variogram sill value is ignored if this parameter is true.
	 * @param epsilon A small value to shift the estimation location a bit.  This trick is used to avoid
	 *        numerical instabilities in certain operations.  Normally this should be zero.
	 */
	static MatrixNXM<double> makeGammaMatrix(DataCellPtrMultiset & samples,
											 GridCell& estimationLocation,
											 VariogramModel *variogramModel,
											 double variogramSill,
											 KrigingType kType = KrigingType::SK,
											 bool returnGamma = false,
											 double epsilon = 0.0 );

    /**
     *  Returns a list of valued grid cells, ordered by topological proximity to the target cell.
     */
	static void getValuedNeighborsTopoOrdered(GridCell &cell,
															int numberOfSamples,
															int nColsAround,
															int nRowsAround,
															int nSlicesAround,
															bool hasNDV,
															double NDV,
															GridCellPtrMultiset & list);
	/** Creates the P matrix for Factorial Kriging.
	 * see theory in Ma et al. (2014) - Factorial kriging for multiscale modelling.
	 * @param nsamples Number of samples for the kriging operation.
	 * @param nst Number of structures in the variogram ( TODO: check whether this includes the nugget effect ).
	 * @param kType Kriging type.  Must match the kType used to call makeCovMatrix() and makeGammaMatrix() so the
	 *        returned matrix is multiplication compatible with the other matrices in the kriging system.
	 */
	static MatrixNXM<double> makePmatrixForFK(int nsamples, int nst, KrigingType kType );

	/** Creates the p matrix for Factorial Kriging.
	 * see theory in Ma et al. (2014) - Factorial kriging for multiscale modelling.
	 */
	static MatrixNXM<double> makepMatrixForFK(int nst);
};

#endif // GEOSTATSUTILS_H
