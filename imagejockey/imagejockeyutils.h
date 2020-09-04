#ifndef IMAGEJOCKEYUTILS_H
#define IMAGEJOCKEYUTILS_H

#include <cassert>
#include <cstdint>
#include <QString>
#include <QPointF>
#include <vtkSmartPointer.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <numeric>
#include "ijmatrix3x3.h"
#include "spectral/spectral.h"
#include <vtkImageData.h>

class IJSpatialLocation;
class IJAbstractCartesianGrid;
class vtkPolyData;

class ImageJockeyUtils
{
public:
	ImageJockeyUtils();

    /** The math constant PI. */
    static const long double PI;

    /** Constant used to convert degrees to radians. */
    static const long double PI_OVER_180;

	/** Constant used to convert radians to degrees. */
	static const long double _180_OVER_PI;

    /**
	 * Perfoms a reliable way to compare floating-point values.
	 * credit: Bruce Dawson
	 * 32-bit version source: http://www.cygnus-software.com/papers/comparingfloats/comparingfloats.htm
	 * 64-bit version of almostEqual2sComplement by GammaRay authors.
	 */
	inline static bool almostEqual2sComplement(double A, double B, int maxUlps)
	{
		// Make sure maxUlps is non-negative and small enough that the
		// default NAN won't compare as equal to anything.
		//<cassert>'s assert doesn't accept longs
		// assert(maxUlps > 0 && maxUlps < 2 * 1024 * 1024 * 1024 * 1024 * 1024);
		assert(maxUlps > 0 && maxUlps < 4 * 1024 * 1024);
		int64_t aLong = *reinterpret_cast<int64_t *>(&A); // use the raw bytes from the
														  // double to make a long int
														  // value (type punning)
		// Make aLong lexicographically ordered as a twos-complement long
		if (aLong < 0)
			aLong = 0x8000000000000000 - aLong;
		// Make bLong lexicographically ordered as a twos-complement long
		int64_t bLong = *reinterpret_cast<int64_t *>(&B); // use the raw bytes from the
														  // double to make a long int
														  // value (type punning)
		if (bLong < 0)
			bLong = 0x8000000000000000 - bLong;
		int64_t longDiff = (aLong - bLong) & 0x7FFFFFFFFFFFFFFF;
		if (longDiff <= maxUlps)
			return true;
		return false;
	}


	/** Returns decibels of an input value with respect to a reference value.
	 * Examples.: dBm is defined as the decibel level of value in milliwatts with respect
	 * to 1mW.
	 *            dBi is defined as the gain in decibels of an antenna with respect to the
	 * ideal isotropic antenna.
	 * refLevel cannot be zero or a divison by zero error ensues.
	 * epsilon The smallest allowed absolute value as to avoid large negative results or
	 * even -inf (value = 0.0).
	 * @param scaleFactor Use 10.0 for most applications. Use 20.0 for power (square law) applications.
	 */
	static double dB(double value, double refLevel, double epsilon, double scaleFactor = 10.0);

    /** Returns the transform matrix corresponding to the given anisotropy ellipse.
     * The three angles follow the GSLib convention (angles in degrees).
     * E.g.: azimuth 90.0 means that the semi-major axis
     * points to East (x-axis aligned) and increases clockwise (geologist's convention).
     * Positive dip angles are "nose up" and posivite roll angles are "bank to right", following
     * aircraft manuevers analogy.
     */
    static IJMatrix3X3<double> getAnisoTransform( double aSemiMajor, double aSemiMinor, double aSemiVert,
                                                  double azimuth, double dip, double roll );

    /** Transforms the 3x1 vector-column (a1, a2, a3) with the given 3x3 matrix. */
    static void transform( IJMatrix3X3<double>& t, double& a1, double& a2, double& a3 );

    /** Returns a text containing a better human readable value.
     * E.g.: 25000000 results in "25M".  Client code can then append a basic unit: "25M" +
     * "Hz" to make "25MHz".
     * This function supports multiplier suffixes from pico (p) to Exa (E).  More can be
     * added with small changes
     * to its code.
     */
    static QString humanReadable(double value);

    /** Mirrors a geometry (given as a set of QPointF) with respect a given point in
     * space. */
    static void mirror2D(QList<QPointF> &points, const IJSpatialLocation &point);

    /** Tests whether the given 2D location lies within the given 2D bounding box. */
    static bool isWithinBBox(double x, double y, double minX, double minY, double maxX,
                             double maxY);

    /** This function performs transforms in the passed data:
     *     a) Shift low frequencies from the center to the corners of the grid.
     *     b) Transform the complex numbers in polar form (a cis b) to Cartesian form (a + bi).
     *     c) Changes the grid scan order from GSLib convention (inner I, mid J, outer K) to FFTW3 convention
     *        (inner K, mid J, outer I)
     * Low frequencies in the center and complex numbers in polar form are better for interpretation of data,
     * but that presentation is not compatible with FFTW3 algorithms.
     * The input grids ( gridWithAmplitudes and gridWithPhases ) can point to the same object (multivariate grid).
     * The input grids must have the same rank, like compatible matrices for addition.
     * Returns true if sucessfull.
     */
    static bool prepareToFFTW3reverseFFT(IJAbstractCartesianGrid *gridWithAmplitudes,
                                         uint indexOfVariableWithAmplitudes,
                                         IJAbstractCartesianGrid *gridWithPhases,
                                         uint indexOfVariableWithPhases,
                                         spectral::complex_array &output );

    /** Generates a unique file name in the given directory.
     * Returns the complete path to it.  File extension must include the dot.
     */
    QString generateUniqueFilePathInDir(const QString directory, const QString file_extension);

    /**
	 * Populates a vtkImageData object with the data from a spectral::array object.
	 * Client code must create the vtkImageData object beforehand with a call to
	 * vtkSmartPointer<vtkImageData>::New(), for example.
     * The client code must also define the grid geometry such as cell sizes and origin
     * (default are 1,1,1 and 0,0,0 respectively) because spectral::array only has only data values,
     * not geometry.
     * @param functor A lambda or non-class function used to transform the values (e.g. absolute values).
     *                This functor must be made such it accepts a sole double value as parameter and returns a double.
     *                If you don't need to transform the values, simply call makeVTKImageDataFromSpectralArray without
     *                the the third parameter.
     *                Code example:
     *                       int g(int x) {
     *                           return std::abs( x );
     *                       }
     *                       void Foo::foomethod() {
     *                           auto lambda = [] (double x) { return std::abs(x); };
     *                           makeVTKImageDataFromSpectralArray( out, in, lambda ); //pass lambda
     *                           makeVTKImageDataFromSpectralArray( out, in, g );      //pass function
     *                       }
	 */
    template<typename TransformFunctor>
    static void makeVTKImageDataFromSpectralArray( vtkImageData* out,
                                                   const spectral::array& in,
                                                   TransformFunctor functor ) {
        out->SetExtent(0, in.M()-1, 0, in.N()-1, 0, in.K()-1); //extent (indexes) of GammaRay grids start at i=0,j=0,k=0
        out->AllocateScalars(VTK_DOUBLE, 1); //each cell will contain one double value.
        int* extent = out->GetExtent();

        for (int k = extent[4]; k <= extent[5]; ++k){
            for (int j = extent[2]; j <= extent[3]; ++j){
                for (int i = extent[0]; i <= extent[1]; ++i){
                    double* pixel = static_cast<double*>(out->GetScalarPointer(i,j,k));
                    pixel[0] = functor( in( i, j, k ) );
                }
            }
        }
    }

    /**
     * An overload of makeVTKImageDataFromSpectralArray() that builds a default
     * lambda that does not transform the values.
     */
    static void makeVTKImageDataFromSpectralArray( vtkImageData* out,
                                                   const spectral::array& in ) {
        auto f = [] (double x) { return x; };
        makeVTKImageDataFromSpectralArray( out, in, f );
    }

    /**
     * Rasterizes the vector geometry (as a vtkPolyData) into a spectral::array grid object.
     * The grid dimensions of the spectral::array object is set or re-set to acommodate the polygonal
     * object's bounding box.
     * The three r* parameters are resolutions at which the polygonal object will be rasterized.
	 * 	Further reading : https://www.vtk.org/pipermail/vtkusers/2008-May/046477.html
	 *	                  Hybrid/Testing/Tcl/TestImageStencilWithPolydata.tcl.
	 *	                  I wouldn't advise using a generic filter like vtkSelectEnclosedPoints because it will be quite slow.
	 *                    https://www.vtk.org/Wiki/VTK/Examples/Cxx/PolyData/PolyDataContourToImageData
	 *                    https://www.vtk.org/Wiki/VTK/Examples/PolyData/PolyDataToImageData
     */
    static void rasterize(spectral::array& out, vtkPolyData *in , double rX, double rY, double rZ);

	/**
	 * Computes the isocontours/isosurfaces of the values in the passed grid.
	 */
	static vtkSmartPointer<vtkPolyData> computeIsosurfaces(const spectral::array& in,
														   int nCountours,
														   double minValue,
														   double maxValue);

    /** Keeps only the closed polyognals in the given polygonal object.
     * After the call, the smart pointer passed refers to another new vtkPolyData object without open poly lines.
     */
    static void removeOpenPolyLines( vtkSmartPointer<vtkPolyData>& polyDataToModify );

    /** Keeps only the polyognals whose center of mass is close to the passed coordinate.
     * After the call, the smart pointer passed refers to another new vtkPolyData object without open poly lines.
	 * @parameter numberOfVertexesThreshold The minimum number of vertexes for a polyine to be acceptable.
	 *                                      Setting zero causes no rejection.
     */
	static void removeNonConcentricPolyLines(vtkSmartPointer<vtkPolyData>& polyDataToModify,
											  double centerX,
											  double centerY,
											  double centerZ,
											  double toleranceRadius,
											  int numberOfVertexesThreshold = 0);

    /**
     * Returns the X, Y, Z, radius ( sqrt(X^2 + Y^2 + X^2) ) and scalar (if present) of the vertexes as
     * time-series data, that is: as functions of the independent variable phase.
     * The returned X, Y and Z coordiantes are with respect to a reference centerX, centerY, centerZ
     * location, which is normally the center of the closed polygons.
     * The outer vector corresponds to the polygons found in the input vtkPolyData object.
     * The inner vector corresponds to the vertexes found in a polygon.
     * @param repeats Repeats the unwinding a number of times so the periodicity can be better assessed.
     *                Default number is 1.
     */
    struct TimeSeriesEntry{ double phase, x, y, z, radius, scalar; };
    static std::vector< std::vector< TimeSeriesEntry > > unwindAsTimeSeries(const vtkSmartPointer<vtkPolyData>& polyData,
                                                            double centerX, double centerY, double centerZ,
                                                            int repeats = 1);

    /** Fits ellipses to each poly line in the input poly data.  The ellipses are stored
	 * in another poly data object to be referenced in passed VTK smart pointer.  If the passed
	 * pointer is null, no ellipses poly is generated and the function only returns the ellipses stats (faster execution).
	 * @param mean_error Filled with the mean fitness error of all ellipses.
	 * @param max_error Filled with the largest fitness error of all ellipses.
	 * @param sum_error Filled with the sum of fitness errors of all ellipses.
     * @param angle_variance Filled with the variance of the ellipses' angles.
     * @param ratio_variance Filled with the variance of the ellipses' semi-axes ratios.
     * @param angle_mean Filled with the mean of the ellipses' angles.
     * @param ratio_mean Filled with the mean of the ellipses' semi-axes ratios.
	 * @param nSkipOutermost Do not fit ellipses to the n outermost polylines.  Set zero to fit ellipses to all poly lines.
	 *                       This is useful to lower the influence of anisotropy too distant from the center of variographic maps.
     */
	static void fitEllipses(const vtkSmartPointer<vtkPolyData>& polyData,
							vtkSmartPointer<vtkPolyData>& ellipses ,
							double &mean_error, double &max_error, double &sum_error,
							double &angle_variance, double &ratio_variance, double &angle_mean, double &ratio_mean,
							int nSkipOutermost );

    /**
     * Computes the ellipse parameters from the factors of the ellipse implicit equation in the form
     * Ax^2 + Bxy + Cy^2 + Dx + Ey + F = 0, commonly yielded by ellipse-fitting algorithms.
     * The parameters passed by reference are the output parameters.
	 *
     * CREDIT: Ruobing Li http://miracle21.blogspot.com/2011/12/ellipse-fitting-and-parameter.html
	 *
	 * @note This function is likely wrong since it does not use parameter F.
	 */
    static void getEllipseParametersFromImplicit(double A, double B, double C, double D, double E, double F,
                                                  double& semiMajorAxis,
                                                  double& semiMinorAxis,
                                                  double& rotationAngle,
                                                  double& centerX,
                                                  double& centerY );

	/** Does the same as getEllipseParametersFromImplicit() but with formulae taken from Wikipedia:
	 * https://en.wikipedia.org/wiki/Ellipse (Ellipse as quadric)
	 */
	static void getEllipseParametersFromImplicit2(double A, double B, double C, double D, double E, double F,
												  double& semiMajorAxis,
												  double& semiMinorAxis,
												  double& rotationAngle,
												  double& centerX,
												  double& centerY );

    /**
     * Fits an ellipse to a given set of spatial coordinates.  The parameters passed by reference are the
     * output parameters.  The ellipse parameters are the factors of its implicit equation
     * Ax^2 + Bxy + Cy^2 + Dx + Ey + F = 0.  Use the getEllipseParametersFromImplicit() method to obtain
     * its geometric parameters such as semi-axes, center, etc.
     * This function uses the direct least squares method proposed by Fitzgibbon et al (1996), which yields
     * the fitted ellipse in a single step (non-iterative) and assures an ellipse quadric
     * (e.g. not hyperbola, parabola, etc.) for bad data.
	 * The fitness error (normalized) is also returned.
     */
	static void ellipseFit(const spectral::array& aX,
							const spectral::array& aY,
							double& A, double& B, double& C, double& D, double& E, double& F, double & fitnessError);

    /**
     * Computes the variance and mean of a collection of values of some type.
     */
    template <class T>
    static void getStats( const std::vector<T>& values, double& variance, double& mean ){
        mean = std::accumulate( values.begin(), values.end(), 0.0 ) / values.size();
        std::vector<T> diff( values.size() );
        std::transform( values.begin(), values.end(), diff.begin(), [mean](T x) { return x - mean; });
        double squaredSum = std::inner_product( diff.begin(), diff.end(), diff.begin(), 0.0 );
        double stdev = std::sqrt(squaredSum / (double)values.size());
        variance = stdev * stdev;
    }

	/**
	 * Computes the azimuth, in degrees, of the given (x,y) location with respect to some other
	 * location (centerX, centerY).  The azimuth returned follows the GSLib convention, or geologist's convention:
	 * Zero azimuth points to north and increases clockwise.
	 * @param halfAzimuth If true, instead of a 0-360 range, the azimuth resets to zero after N180E (south), which makes
	 *                    antipodal points have the same half-azimuth values.
	 * @note If the internal arctangent call results in infinity (very near or at the center) the returned azimuth value is zero.
	 */
	static double getAzimuth( double x, double y, double centerX, double centerY, bool halfAzimuth = false );

    /**
     * Interpolates invalid values ( std::isfinite() returns false ) from valid values in the passed array.
     * The returned array has the same dimensions of the input array.
     * Interpolation method is Shepard's (inverse distance weighted of all data points).
     * This method is slow if the array has too many valid values.
     * @param inputData array of data values.  Number of data elements must be nI * nJ * nK (see gridMesh parameter).
     * @param gridMesh an object containing grid mesh definition, that is,
     *        origin (X0, Y0, Z0), cell sizes (dX, dY, dZ) and cell count (nI, nJ, nK).
     * @param powerParameter The inverse distance power.  The algorithm is optimized for 2.0.
     * @param maxDistanceFactor Determines the maximum distance beyond which no samples are considered for the interpolation.
     *                          1.0 means no limit, hence all samples are used (slower, but without serach neighborhood artifacts).
     *                          Use values between 0.0 and 1.0 to set a maximum distance.
     * @param nullValue Value to be used in places impossible to interpolate due to lack of samples with valid values.
     */
    static spectral::array interpolateNullValuesShepard(const spectral::array& inputData,
                                                        IJAbstractCartesianGrid& gridMesh,
                                                        double powerParameter = 2.0,
                                                        double maxDistanceFactor = 1.0,
                                                        double nullValue = std::numeric_limits<double>::quiet_NaN() );


    /**
    * Interpolates invalid values ( std::isfinite() returns false ) from valid values in the passed array.
    * The returned array has the same dimensions of the input array.
    * Interpolation method is Thin Plate Spline ("Approximation Methods for Thin Plate Spline Mappings and Principal Warps"
    * (Donato G and Belongie S, 2002), thus the input data must be 2D (only data at first z-slice will be processed).
    * This implementation is adapted from the original stand-alone C++ code by Jarno Elonen: https://elonen.iki.fi/code/tpsdemo/index.html
    * In this implementation, the height variable is Z, instead of Elonen's Y.
    * This method is slow if the array has too many valid values.
    * @param inputData array of data values.  Number of data elements must be nI * nJ (see gridMesh parameter).
    * @param gridMesh an object containing grid mesh definition, that is,
    *        origin (X0, Y0), cell sizes (dX, dY) and cell count (nI, nJ)).
    * @param lambda Set to 0.0 to force the spline pass through all points.  Set to near-infinity for a least-squares plane.
    *               Set to values in-between for a relaxed interpolation of a smooth spline, balancing misfit and smoothness.
    * @param status Check this value for execution termination status: 0 = OK; 1 = singular matrix, 2 = premature ending, 3 = innacurate solution.
    *               Do not use the returned array if the execution fails.
    */
    static spectral::array interpolateNullValuesThinPlateSpline( const spectral::array& inputData,
                                                                 IJAbstractCartesianGrid& gridMesh,
                                                                 double lambda,
                                                                 int& status );

    /** Skeletonizes gridded data so only values along thin lines remain. */
    static spectral::array skeletonize( const spectral::array& inputData );

    /** Performs the mean filter on the passed gridded data.
     * @param windowSize The size of the convolution kernel in cells.  Zero results in a simply copy of values.
     */
    static spectral::array meanFilter(const spectral::array& inputData, int windowSize);

    /** Performs the median filter on the passed gridded data.
     * @param windowSize The size of the convolution kernel in cells.  Zero results in a simply copy of values.
     */
    static spectral::array medianFilter(const spectral::array& inputData, int windowSize);

    /** Performs the Gaussian filter on the passed gridded data.
     * @param sigma The standard deviation (in cell count units) parameter of the Gaussian-bell-shaped
     *              kernel of the filter.
     */
    static spectral::array gaussianFilter(const spectral::array& inputData, float sigma);
};

#endif // IMAGEJOCKEYUTILS_H
