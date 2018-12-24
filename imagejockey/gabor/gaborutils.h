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
    typedef itk::GaussianInterpolateImageFunction<ImageType, realType> GaussianInterpolatorType;
    typedef itk::Euler2DTransform<realType> TransformType;
    typedef itk::ResampleImageFilter<ImageType, ImageType, realType> ResamplerType;
    typedef itk::ConvolutionImageFilter<ImageType> ConvolutionFilterType;

    static ImageTypePtr createITKImageFromCartesianGrid(IJAbstractCartesianGrid &input,
                                                         int variableIndex );

    /** The azimuth follows the geologist's convention. 0 degrees = North; 90 deg. = East. etc.
     * Due to technical constraints of the ITK library, the azimuth must be between 3 and 177
     * degrees, including these values.
     */
    static ImageTypePtr computeGaborResponse( double frequency,
                                              double azimuth,
                                              const ImageTypePtr inputImage );

};

#endif // GABORUTILS_H
