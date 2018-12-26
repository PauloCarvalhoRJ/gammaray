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
#include <itkRescaleIntensityImageFilter.hxx>


/**
 * The GaborUtils class organizes processing units for Gabor-related workflows.
 */
class GaborUtils
{
public:
    GaborUtils();

    //typedfs and other definitions
    static const unsigned int gridDim = 2;
    typedef float realType;
    typedef itk::Image<realType, gridDim> ImageType;
    typedef ImageType::Pointer ImageTypePtr;
    typedef itk::GaborImageSource<ImageType> GaborSourceType;
    typedef GaborSourceType::Pointer GaborSourceTypePtr;
    typedef itk::GaussianInterpolateImageFunction<ImageType, realType> GaussianInterpolatorType;
    typedef itk::Euler2DTransform<realType> TransformType;
    typedef itk::ResampleImageFilter<ImageType, ImageType, realType> ResamplerType;
    typedef itk::ConvolutionImageFilter<ImageType> ConvolutionFilterType;

    static ImageTypePtr createITKImageFromCartesianGrid(IJAbstractCartesianGrid &input,
                                                         int variableIndex );

    /** The azimuth follows the geologist's convention. 0 degrees = North; 90 deg. = East. etc.
     * Due to technical constraints of the ITK library, the azimuth must be between 3 and 177
     * degrees, including these values.
     * @param frequency Sets which frequency of the 2D sine wave inside the 2D Gaussian window.
     * @param azimuth Sets the direction (degrees) of the major axis of the 2D Gaussian window.
     * @param meanMajorAxis Sets the center of the 2D Gaussian window.  Relates to phase.
     * @param meanMinorAxis Sets the center of the 2D Gaussian window.  Relates to phase.
     * @param sigmaMajorAxis Sets the how wide is the 2D Gaussian window along the main axis.
     * @param sigmaMinorAxis Sets the how wide is the 2D Gaussian window along the secondary axis.
     * @param kernelSizeI The size of the convolution kernel in number of cells in E-W direction.
     * @param kernelSizeJ The size of the convolution kernel in number of cells in N-S direction.
     */
    static ImageTypePtr computeGaborResponse( double frequency,
                                              double azimuth,
                                              double meanMajorAxis,
                                              double meanMinorAxis,
                                              double sigmaMajorAxis,
                                              double sigmaMinorAxis,
                                              int kernelSizeI,
                                              int kernelSizeJ,
                                              const ImageTypePtr inputImage );

    /**
     * Creates a 255 x 255 Gabor template kernel object.  Normally it is downscaled (e.g. 20 x 20)
     * and rotated towards a desired azimuth to produce a practical kernel for using in convolutions.
     * @param frequency Sets which frequency of the 2D sine wave inside the 2D Gaussian window.
     * @param meanMajorAxis Sets the center of the 2D Gaussian window.  Relates to phase.
     * @param meanMinorAxis Sets the center of the 2D Gaussian window.  Relates to phase.
     * @param sigmaMajorAxis Sets the how wide is the 2D Gaussian window along the main axis.
     * @param sigmaMinorAxis Sets the how wide is the 2D Gaussian window along the secondary axis.
     */
    static GaborSourceTypePtr createGabor2D( double frequency,
                                             double meanMajorAxis,
                                             double meanMinorAxis,
                                             double sigmaMajorAxis,
                                             double sigmaMinorAxis );

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
     */
    static ImageTypePtr createGaborKernel( double frequency,
                                           double azimuth,
                                           double meanMajorAxis,
                                           double meanMinorAxis,
                                           double sigmaMajorAxis,
                                           double sigmaMinorAxis,
                                           int kernelSizeI,
                                           int kernelSizeJ );

};

#endif // GABORUTILS_H