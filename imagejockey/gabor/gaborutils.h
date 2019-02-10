#ifndef GABORUTILS_H
#define GABORUTILS_H

#include "imagejockey/ijabstractcartesiangrid.h"
#include <itkGaborImageSource.h>
#include <itkConvolutionImageFilter.h>
#include <itkGaussianInterpolateImageFunction.h>
#include <itkEuler2DTransform.h>
#include <itkResampleImageFilter.h>
#include <itkImageFileWriter.hxx>
#include <itkPNGImageIOFactory.h>
#include <itkCastImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>

/**
 * The GaborUtils class organizes processing units for Gabor-related workflows.
 */
class GaborUtils
{
public:
    GaborUtils();

    //typedfs and other definitions
    static const unsigned int gridDim = 2;
    static const unsigned int gaborKernelMaxNI = 255;
    static const unsigned int gaborKernelMaxNJ = 255;
    typedef float realType;
    typedef itk::Image<realType, gridDim> ImageType;
    typedef ImageType::Pointer ImageTypePtr;
    typedef itk::GaborImageSource<ImageType> GaborSourceType;
    typedef GaborSourceType::Pointer GaborSourceTypePtr;
    typedef itk::GaussianInterpolateImageFunction<ImageType, realType> GaussianInterpolatorType;
    typedef itk::Euler2DTransform<realType> TransformType;
    typedef itk::ResampleImageFilter<ImageType, ImageType, realType> ResamplerType;
    typedef itk::ConvolutionImageFilter<ImageType> ConvolutionFilterType;

    /**
     * Creates an ITK image object from the grid object of ImageJockey framework.
     * The image is filled with the values of the given variable (by index).
     */
    static ImageTypePtr createITKImageFromCartesianGrid(IJAbstractCartesianGrid &input,
                                                         int variableIndex );

    /**
     * Creates an ITK image object from the grid object of ImageJockey framework.
     * The values are initialized to zero.
     */
    static ImageTypePtr createEmptyITKImageFromCartesianGrid(IJAbstractCartesianGrid &input );

    /**
     * Convolves the input image against a Gabor kernel defined by the input parameters.
     * The azimuth follows the geologist's convention. 0 degrees = North; 90 deg. = East. etc.
     * The result is an image containing the response which has the same grid cell count
     * of the input image.  Recall that a convolution is a cell-wise operation, thus, grid
     * geometry parameters such as origin and cell sizes are ignored, therefore, frequency
     * is relative to distances in cell counts.
     *
     * @param frequency Sets which frequency of the 2D sine wave inside the 2D Gaussian window.
     * @param azimuth Sets the direction (degrees) of the major axis of the 2D Gaussian window.
     * @param meanMajorAxis Sets the x position of the center of the 2D Gaussian window.  Relates to phase.
     * @param meanMinorAxis Sets the y position of the center of the 2D Gaussian window.  Relates to phase.
     * @param sigmaMajorAxis Sets the how wide is the 2D Gaussian window along the main axis.
     * @param sigmaMinorAxis Sets the how wide is the 2D Gaussian window along the secondary axis.
     * @param kernelSizeI The size of the convolution kernel in number of cells in E-W direction.
     * @param kernelSizeJ The size of the convolution kernel in number of cells in N-S direction.
     * @param imaginaryPart If true, the response is the imaginary part of the transform.
     */
    static ImageTypePtr computeGaborResponse(double frequency,
                                              double azimuth,
                                              double meanMajorAxis,
                                              double meanMinorAxis,
                                              double sigmaMajorAxis,
                                              double sigmaMinorAxis,
                                              int kernelSizeI,
                                              int kernelSizeJ,
                                              const ImageTypePtr inputImage,
                                              bool imaginaryPart );

    /**
     * Does the same as the other computeGaborResponse, but it uses spectral::conv2d()
     * which has a better performance.
     */
    static ImageTypePtr computeGaborResponse(double frequency,
                                              double azimuth,
                                              double meanMajorAxis,
                                              double meanMinorAxis,
                                              double sigmaMajorAxis,
                                              double sigmaMinorAxis,
                                              int kernelSizeI,
                                              int kernelSizeJ,
                                              const spectral::array& inputGrid,
                                              bool imaginaryPart );

    /**
     * Creates a 255 x 255 Gabor template kernel object.  Normally it is downscaled (e.g. 20 x 20)
     * and rotated towards a desired azimuth to produce a practical kernel for using in convolutions.
     * @param frequency Sets which frequency of the 2D sine wave inside the 2D Gaussian window.
     * @param meanMajorAxis Sets the center of the 2D Gaussian window.  Relates to phase.
     * @param meanMinorAxis Sets the center of the 2D Gaussian window.  Relates to phase.
     * @param sigmaMajorAxis Sets the how wide is the 2D Gaussian window along the main axis.
     * @param sigmaMinorAxis Sets the how wide is the 2D Gaussian window along the secondary axis.
     * @param imaginary If true, the kernel serves to compute the imaginary part of the Gabor response.
     */
    static GaborSourceTypePtr createGabor2D( double frequency,
                                             double meanMajorAxis,
                                             double meanMinorAxis,
                                             double sigmaMajorAxis,
                                             double sigmaMinorAxis,
                                             bool imaginary );

    /**
     * Creates a Gabor kernel object usable in convolutions.
     * @param frequency Sets which frequency of the 2D sine wave inside the 2D Gaussian window.
     * @param azimuth Sets the direction (degrees) of the major axis of the 2D Gaussian window.
     * @param meanMajorAxis Sets the center of the 2D Gaussian window.  Relates to phase.
     * @param meanMinorAxis Sets the center of the 2D Gaussian window.  Relates to phase.
     * @param sigmaMajorAxis Sets the how wide is the 2D Gaussian window along the main axis.
     * @param sigmaMinorAxis Sets the how wide is the 2D Gaussian window along the secondary axis.
     * @param kernelSizeI The size of the convolution kernel in number of cells in E-W direction.
     * @param kernelSizeJ The size of the convolution kernel in number of cells in N-S direction.
     * @param imaginary If true, the kernel serves to compute the imaginary part of the Gabor response.
     */
    static ImageTypePtr createGaborKernel( double frequency,
                                           double azimuth,
                                           double meanMajorAxis,
                                           double meanMinorAxis,
                                           double sigmaMajorAxis,
                                           double sigmaMinorAxis,
                                           int kernelSizeI,
                                           int kernelSizeJ,
                                           bool imaginary );

    /**
     * Returns a spectral::array object with data from an ITK image object.
     * Supported grid dimension depends on what is set in the gridDim constant (see top of this header file).
     */
    static spectral::array convertITKImageToSpectralArray( const ImageType& input );

    /**
    * Returns a ITK image object object with data from an spectral::array.
    * Supported grid dimension depends on what is set in the gridDim constant (see top of this header file).
    */
    static ImageTypePtr convertSpectralArrayToITKImage( const spectral::array& input );
};

#endif // GABORUTILS_H
