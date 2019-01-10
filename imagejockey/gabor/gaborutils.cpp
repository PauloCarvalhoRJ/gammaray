#include "gaborutils.h"
#include "imagejockey/imagejockeyutils.h"
#include <itkImageDuplicator.h>

GaborUtils::GaborUtils()
{
}

GaborUtils::ImageTypePtr GaborUtils::createITKImageFromCartesianGrid
                         ( IJAbstractCartesianGrid &input, int variableIndex )
{
    double dX = input.getCellSizeI();
    double dY = input.getCellSizeJ();
    double x0 = input.getOriginX();
    double y0 = input.getOriginY();
    unsigned int nI = input.getNI();
    unsigned int nJ = input.getNJ();

    ImageTypePtr output = ImageType::New();
    {
        ImageType::IndexType start;
        start.Fill(0); // = 0,0,0
        ImageType::SizeType size;
        ImageType::SpacingType spacing; spacing[0] = dX; spacing[1] = dY;
        output->SetSpacing( spacing );
        ////////////////////
        ImageType::PointType origin; origin[0] = x0; origin[1] = y0;
        output->SetOrigin( origin );
        size[0] = nI;
        size[1] = nJ;
        ImageType::RegionType region(start, size);
        output->SetRegions(region);
        output->Allocate();
        output->FillBuffer(0);
        for(unsigned int j = 0; j < nJ; ++j)
            for(unsigned int i = 0; i < nI; ++i){
                double value = input.getData( variableIndex, i, j, 0 );
                itk::Index<gridDim> index;
                index[0] = i;
                index[1] = nJ - 1 - j; // itkImage grid convention is different from GSLib's
                output->SetPixel(index, value);
            }
    }
    return output;
}

GaborUtils::ImageTypePtr GaborUtils::createEmptyITKImageFromCartesianGrid(IJAbstractCartesianGrid &input)
{
    double dX = input.getCellSizeI();
    double dY = input.getCellSizeJ();
    double x0 = input.getOriginX();
    double y0 = input.getOriginY();
    unsigned int nI = input.getNI();
    unsigned int nJ = input.getNJ();

    ImageTypePtr output = ImageType::New();
    {
        ImageType::IndexType start;
        start.Fill(0); // = 0,0,0
        ImageType::SizeType size;
        ImageType::SpacingType spacing; spacing[0] = dX; spacing[1] = dY;
        output->SetSpacing( spacing );
        ////////////////////
        ImageType::PointType origin; origin[0] = x0; origin[1] = y0;
        output->SetOrigin( origin );
        size[0] = nI;
        size[1] = nJ;
        ImageType::RegionType region(start, size);
        output->SetRegions(region);
        output->Allocate();
        output->FillBuffer(0);
    }
    return output;
}

GaborUtils::ImageTypePtr GaborUtils::computeGaborResponse( double frequency,
                                                           double azimuth,
                                                           double meanMajorAxis,
                                                           double meanMinorAxis,
                                                           double sigmaMajorAxis,
                                                           double sigmaMinorAxis,
                                                           int kernelSizeI,
                                                           int kernelSizeJ,
                                                           const ImageTypePtr inputImage,
                                                           bool imaginaryPart )
{

    GaborUtils::ImageTypePtr kernel = GaborUtils::createGaborKernel( frequency,
                                                                     azimuth,
                                                                     meanMajorAxis,
                                                                     meanMinorAxis,
                                                                     sigmaMajorAxis,
                                                                     sigmaMinorAxis,
                                                                     kernelSizeI,
                                                                     kernelSizeJ,
                                                                     imaginaryPart );

    // Convolve the input image against the resampled gabor image kernel.
    typename ConvolutionFilterType::Pointer convoluter = ConvolutionFilterType::New();
    {
        convoluter->SetInput( inputImage );
        convoluter->SetKernelImage( kernel );
        convoluter->NormalizeOn();
        convoluter->Update();
    }

    return convoluter->GetOutput();
}

GaborUtils::ImageTypePtr GaborUtils::computeGaborResponse(double frequency,
                                                          double azimuth,
                                                          double meanMajorAxis,
                                                          double meanMinorAxis,
                                                          double sigmaMajorAxis,
                                                          double sigmaMinorAxis,
                                                          int kernelSizeI,
                                                          int kernelSizeJ,
                                                          const spectral::array &inputGrid,
                                                          bool imaginaryPart)
{
    GaborUtils::ImageTypePtr kernel = GaborUtils::createGaborKernel( frequency,
                                                                     azimuth,
                                                                     meanMajorAxis,
                                                                     meanMinorAxis,
                                                                     sigmaMajorAxis,
                                                                     sigmaMinorAxis,
                                                                     kernelSizeI,
                                                                     kernelSizeJ,
                                                                     imaginaryPart );

    spectral::array kernelA = GaborUtils::convertITKImageToSpectralArray( *kernel );

    spectral::array result;
    spectral::conv2d( result, inputGrid, kernelA );
    return GaborUtils::convertSpectralArrayToITKImage( result );
}

GaborUtils::GaborSourceTypePtr GaborUtils::createGabor2D(double frequency,
                                                         double meanMajorAxis,
                                                         double meanMinorAxis,
                                                         double sigmaMajorAxis,
                                                         double sigmaMinorAxis,
                                                         bool imaginary)
{
    GaborSourceTypePtr gabor = GaborSourceType::New();
    {
        ImageType::SpacingType spacing; spacing[0] = 1.0; spacing[1] = 1.0;
        gabor->SetSpacing( spacing );
        ////////////////////
        ImageType::PointType origin; origin[0] = 0.0; origin[1] = 0.0;
        gabor->SetOrigin( origin );
        ////////////////////
        ImageType::RegionType::SizeType size; size[0] = gaborKernelMaxNI; size[1] = gaborKernelMaxNJ;
        gabor->SetSize( size );
        ////////////////////
        ImageType::DirectionType direction; direction.SetIdentity();
        gabor->SetDirection( direction );
        ////////////////////
        gabor->SetFrequency( frequency );
        ////////////////////
        gabor->SetCalculateImaginaryPart( imaginary );
        ////////////////////
        GaborSourceType::ArrayType mean;
        mean[0] = meanMajorAxis;
        mean[1] = meanMinorAxis;
        gabor->SetMean( mean );
        ////////////////////
        GaborSourceType::ArrayType sigma;
        sigma[0] = sigmaMajorAxis;
        sigma[1] = sigmaMinorAxis;
        gabor->SetSigma( sigma );
    }
    return gabor;
}

GaborUtils::ImageTypePtr GaborUtils::createGaborKernel(double frequency,
                                                        double azimuth,
                                                        double meanMajorAxis,
                                                        double meanMinorAxis,
                                                        double sigmaMajorAxis,
                                                        double sigmaMinorAxis,
                                                        int kernelSizeI,
                                                        int kernelSizeJ ,
                                                       bool imaginary)
{
    //convert azimuth to an angle in radians and in trigonometric convention
    double azimuthRad = ( - azimuth + 90 ) *  ImageJockeyUtils::PI_OVER_180;

    // Size of the kernel (e.g. 20 x 20 pixels).
    ImageType::RegionType::SizeType kernelSize;
    kernelSize[0] = kernelSizeI;
    kernelSize[1] = kernelSizeJ;

    // Construct the gabor image kernel.
    // The idea is that we construct a gabor image kernel and
    // downsample it to a smaller size for image convolution.
    GaborSourceTypePtr gabor = GaborUtils::createGabor2D( frequency,
                                                          meanMajorAxis,
                                                          meanMinorAxis,
                                                          sigmaMajorAxis,
                                                          sigmaMinorAxis,
                                                          imaginary );
    gabor->Update();


    //debug the full Gabor kernel image
//    {
//        // Rescale the values and convert the image
//        // so that it can be seen as a PNG file
//        typedef itk::Image<unsigned char, 3>  PngImageType;
//        typedef itk::RescaleIntensityImageFilter< ImageType, PngImageType > RescaleType;
//        RescaleType::Pointer rescaler = RescaleType::New();
//        rescaler->SetInput( gabor->GetOutput() );
//        rescaler->SetOutputMinimum(0);
//        rescaler->SetOutputMaximum(255);
//        rescaler->Update();
//        //save the converted umage as PNG file
//        itk::PNGImageIOFactory::RegisterOneFactory();
//        typedef itk::ImageFileWriter< PngImageType > WriterType;
//        WriterType::Pointer writer = WriterType::New();
//        writer->SetFileName("~itkFullGaborKernelImage.png");
//        writer->SetInput( rescaler->GetOutput() );
//        writer->Update();
//        return;
//    }



    //debug the input image
//    {
//        // Rescale the values and convert the image
//        // so that it can be seen as a PNG file
//        typedef itk::Image<unsigned char, 3>  PngImageType;
//        typedef itk::RescaleIntensityImageFilter< ImageType, PngImageType > RescaleType;
//        RescaleType::Pointer rescaler = RescaleType::New();
//        rescaler->SetInput( inputImage );
//        rescaler->SetOutputMinimum(0);
//        rescaler->SetOutputMaximum(255);
//        rescaler->Update();
//        //save the converted umage as PNG file
//        itk::PNGImageIOFactory::RegisterOneFactory();
//        typedef itk::ImageFileWriter< PngImageType > WriterType;
//        WriterType::Pointer writer = WriterType::New();
//        writer->SetFileName("~itkGaborInputImage.png");
//        writer->SetInput( rescaler->GetOutput() );
//        writer->Update();
//        return;
//    }

    // Construct a Gaussian interpolator for the gabor filter resampling
    typename GaussianInterpolatorType::Pointer gaussianInterpolator = GaussianInterpolatorType::New();
    {
        gaussianInterpolator->SetInputImage( gabor->GetOutput() /*inputImage*/ );
        double sigma[gridDim];
        for( unsigned int i = 0; i < gridDim; i++ )
            sigma[i] = 0.8;
        double alpha = 1.0;
        gaussianInterpolator->SetParameters( sigma, alpha );
    }

    // make a linear transform (translation, rotation, etc.) object
    // to manipulate the Gabor kernel.
    TransformType::Pointer transform = TransformType::New();
    {
        //set translation
        TransformType::OutputVectorType translation;
        translation.Fill( 0.0 ); // = 0.0,0.0,0.0 == no translation
        transform->SetTranslation( translation );
        //set center for rotation
        TransformType::InputPointType center;
        for( unsigned int i = 0; i < gridDim; i++ )
        {
            center[i] = gabor->GetOutput()->GetOrigin()[i] +
                        0.5 * gabor->GetOutput()->GetSpacing()[i] *
                      ( gabor->GetOutput()->GetBufferedRegion().GetSize()[i] - 1 );
        }
        transform->SetCenter( center );
        //set rotation angles
        transform->SetRotation( azimuthRad );
    }

    // create an usable kernel image from the Gabor parameter object
    // after transforms are applied
    ResamplerType::Pointer resampler = ResamplerType::New();
    {
        resampler->SetTransform( transform );
        resampler->SetInterpolator( gaussianInterpolator );
        resampler->SetInput( gabor->GetOutput() );
        ImageType::SpacingType spacing;
        for( int i = 0; i < gridDim; ++i )
            spacing[i] = gabor->GetOutput()->GetSpacing()[i] *
                    gabor->GetSize()[i] / kernelSize[i];
        resampler->SetOutputSpacing( spacing );
        resampler->SetOutputOrigin( gabor->GetOutput()->GetOrigin() /*inputImage->GetOrigin()*/ );
        resampler->SetSize( kernelSize );
        resampler->Update();
    }

    // debug the rescaled Gabor kernel
//    {
//        // Rescale the values and convert the image
//        // so that it can be seen as a PNG file
//        typedef itk::Image<unsigned char, 3>  PngImageType;
//        typedef itk::RescaleIntensityImageFilter< ImageType, PngImageType > RescaleType;
//        RescaleType::Pointer rescaler = RescaleType::New();
//        rescaler->SetInput( resampler->GetOutput() );
//        rescaler->SetOutputMinimum(0);
//        rescaler->SetOutputMaximum(255);
//        rescaler->Update();
//        //save the converted umage as PNG file
//        itk::PNGImageIOFactory::RegisterOneFactory();
//        typedef itk::ImageFileWriter< PngImageType > WriterType;
//        WriterType::Pointer writer = WriterType::New();
//        writer->SetFileName("~itkRescaledGaborKernel.png");
//        writer->SetInput( rescaler->GetOutput() );
//        writer->Update();
//        return;
//    }

    return resampler->GetOutput();
}

spectral::array GaborUtils::convertITKImageToSpectralArray(const GaborUtils::ImageType &input)
{
    //get input grid dimensions
    ImageType::RegionType region = input.GetLargestPossibleRegion();
    ImageType::SizeType size = region.GetSize();
    spectral::index nI = size[0];
    spectral::index nJ = 1;
    if( GaborUtils::gridDim > 1 )
        nJ = size[1];
    spectral::index nK = 1;
    if( GaborUtils::gridDim > 2 )
        nK = size[2];

    //transfer values to a new spectral::array object
    spectral::array result( nI, nJ, nK );
    for(unsigned int k = 0; k < nK; ++k)
        for(unsigned int j = 0; j < nJ; ++j)
            for(unsigned int i = 0; i < nI; ++i){
                itk::Index<gridDim> index;
                switch( GaborUtils::gridDim ){
                case 3: index[2] = nK - 1 - k; // itkImage grid convention is different from GSLib's
                case 2: index[1] = nJ - 1 - j; // itkImage grid convention is different from GSLib's
                }
                index[0] = i;
                GaborUtils::realType value = input.GetPixel( index );
                result( i, j, k ) = value;
            }

    return result;
}

GaborUtils::ImageTypePtr GaborUtils::convertSpectralArrayToITKImage( const spectral::array& input )
{
    double dX = 1.0;
    double dY = 1.0;
    double x0 = 0.0;
    double y0 = 0.0;
    unsigned int nI = input.M();
    unsigned int nJ = input.N();

    ImageTypePtr output = ImageType::New();
    {
        ImageType::IndexType start;
        start.Fill(0); // = 0,0,0
        ImageType::SizeType size;
        ImageType::SpacingType spacing; spacing[0] = dX; spacing[1] = dY;
        output->SetSpacing( spacing );
        ////////////////////
        ImageType::PointType origin; origin[0] = x0; origin[1] = y0;
        output->SetOrigin( origin );
        size[0] = nI;
        size[1] = nJ;
        ImageType::RegionType region(start, size);
        output->SetRegions(region);
        output->Allocate();
        output->FillBuffer(0);
        for(unsigned int j = 0; j < nJ; ++j)
            for(unsigned int i = 0; i < nI; ++i){
                double value = input( i, j, 0 );
                itk::Index<gridDim> index;
                index[0] = i;
                index[1] = nJ - 1 - j; // itkImage grid convention is different from GSLib's
                output->SetPixel(index, value);
            }
    }
    return output;
}

