#include "gaborfilterdialog.h"
#include "ui_gaborfilterdialog.h"
#include "imagejockey/ijabstractvariable.h"
#include "imagejockey/ijabstractcartesiangrid.h"
#include "spectral/spectral.h"
#include <itkGaborImageSource.h>
#include <itkConvolutionImageFilter.h>
#include <itkGaussianInterpolateImageFunction.h>
#include <itkEuler3DTransform.h>
#include <itkResampleImageFilter.h>
#include <itkImageFileWriter.hxx>
#include <itkPNGImageIOFactory.h>

GaborFilterDialog::GaborFilterDialog(IJAbstractCartesianGrid *inputGrid,
                                     uint inputVariableIndex,
                                     QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GaborFilterDialog),
    m_inputGrid( inputGrid ),
    m_inputVariableIndex( inputVariableIndex )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle( "Gabor Filter Dialog" );
}

GaborFilterDialog::~GaborFilterDialog()
{
    delete ui;
}

void GaborFilterDialog::onPerformGaborFilter()
{
    //typedfs and other definitions
    const unsigned int gridDim = 3;
    typedef float realType;
    typedef itk::Image<realType, gridDim> ImageType;
    typedef itk::GaborImageSource<ImageType> GaborSourceType;
    typedef itk::GaussianInterpolateImageFunction<ImageType, realType> GaussianInterpolatorType;
    typedef itk::Euler3DTransform<realType> TransformType;
    typedef itk::ResampleImageFilter<ImageType, ImageType, realType> ResamplerType;
    typedef itk::ConvolutionImageFilter<ImageType> ConvolutionFilterType;

    ///----------------------------user-defined Gabor parameters------------------------------
    double frequency = 0.001;
    double azimuth = 0.0;
    // Size of the kernel.
    ImageType::RegionType::SizeType kernelSize;
    for( unsigned int i = 0; i < gridDim; ++i )
        kernelSize[i] = 20;
    ///---------------------------------------------------------------------------------------

    //get grid geometry
    double dX = m_inputGrid->getCellSizeI();
    double dY = m_inputGrid->getCellSizeJ();
    double dZ = m_inputGrid->getCellSizeK();
    double x0 = m_inputGrid->getOriginX();
    double y0 = m_inputGrid->getOriginY();
    double z0 = m_inputGrid->getOriginZ();
    unsigned int nI = m_inputGrid->getNI();
    unsigned int nJ = m_inputGrid->getNJ();
    unsigned int nK = m_inputGrid->getNK();

    // Construct the gabor image kernel.
    // The idea is that we construct a gabor image kernel and
    // downsample it to a smaller size for image convolution.
    GaborSourceType::Pointer gabor = GaborSourceType::New();
    {
        ImageType::SpacingType spacing; spacing[0] = 1.0; spacing[1] = 1.0; spacing[2] = 1.0;
        gabor->SetSpacing( spacing );
        ////////////////////
        ImageType::PointType origin; origin[0] = 0.0; origin[1] = 0.0; origin[2] = 0.0;
        gabor->SetOrigin( origin );
        ////////////////////
        ImageType::RegionType::SizeType size; size[0] = 255; size[1] = 255; size[2] = 255;
        gabor->SetSize( size );
        ////////////////////
        ImageType::DirectionType direction; direction.SetIdentity();
        gabor->SetDirection( direction );
        ////////////////////
        gabor->SetFrequency( frequency );
        ////////////////////
        gabor->SetCalculateImaginaryPart( true );
        ////////////////////
        GaborSourceType::ArrayType mean;
        for( unsigned int i = 0; i < gridDim; i++ )
            mean[i] = origin[i] + 0.5 * spacing[i] * static_cast<realType>( size[i] - 1 );
        gabor->SetMean( mean );
        ////////////////////
        GaborSourceType::ArrayType sigma;
        sigma[0] = 50.0;
        sigma[1] = 75.0;
        sigma[2] = 75.0;
        gabor->SetSigma( sigma );
    }
    gabor->Update();


    //debug the full Gabor kernel image
    CONVERT_OUTPUT_TO_IMAGE_WITH_CHAR_OR_USHORT;
    {
        itk::PNGImageIOFactory::RegisterOneFactory();
        typedef  itk::ImageFileWriter< ImageType  > WriterType;
        WriterType::Pointer writer = WriterType::New();
        writer->SetFileName("~itkFullGaborKernelImage.png");
        writer->SetInput(gabor->GetOutput());
        writer->Update();
        return;
    }


    // create an ITK image object from input grid data.
    ImageType::Pointer inputImage = ImageType::New();
    {
        ImageType::IndexType start;
        start.Fill(0); // = 0,0,0
        ImageType::SizeType size;
        ImageType::SpacingType spacing; spacing[0] = dX; spacing[1] = dY; spacing[2] = dZ;
        inputImage->SetSpacing( spacing );
        ////////////////////
        ImageType::PointType origin; origin[0] = x0; origin[1] = y0; origin[2] = z0;
        inputImage->SetOrigin( origin );
        //size.Fill( 100 );
        size[0] = nI;
        size[1] = nJ;
        size[2] = nK;
        ImageType::RegionType region(start, size);
        inputImage->SetRegions(region);
        inputImage->Allocate();
        inputImage->FillBuffer(0);
        for(unsigned int k = 0; k < nK; ++k)
            for(unsigned int j = 0; j < nJ; ++j)
                for(unsigned int i = 0; i < nI; ++i){
                    double value = m_inputGrid->getData( m_inputVariableIndex, i, j, k );
                    itk::Index<gridDim> index;
                    index[0] = i;
                    index[1] = nJ - 1 - j; // itkImage grid convention is different from GSLib's
                    index[2] = nK - 1 - k;
                    inputImage->SetPixel(index, value);
                }
    }

    // Construct a Gaussian interpolator for the gabor filter resampling
    typename GaussianInterpolatorType::Pointer gaussianInterpolator = GaussianInterpolatorType::New();
    {
        gaussianInterpolator->SetInputImage( inputImage );
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
                        gabor->GetOutput()->GetSpacing()[i] *
                      ( gabor->GetOutput()->GetBufferedRegion().GetSize()[i] - 1 );
        }
        transform->SetCenter( center );
        //set rotation angles
        transform->SetRotation( 0.0, 0.0, azimuth );
    }

    // create an usable kernel image from the Gabor parameter object
    // after transforms are applied
    ResamplerType::Pointer resampler = ResamplerType::New();
    {
        resampler->SetTransform( transform );
        resampler->SetInterpolator( gaussianInterpolator );
        resampler->SetInput( gabor->GetOutput() );
        // The output spacing and origin are irrelevant in the convolution
        // calculation.
        resampler->SetOutputSpacing( gabor->GetOutput()->GetSpacing() );
        resampler->SetOutputOrigin( inputImage->GetOrigin() );
        resampler->SetSize( kernelSize );
        resampler->Update();
    }

    // Convolve the input image against the resampled gabor image kernel.
    typename ConvolutionFilterType::Pointer convoluter = ConvolutionFilterType::New();
    {
        convoluter->SetInput( inputImage );
        convoluter->SetKernelImage( resampler->GetOutput() );
        convoluter->NormalizeOn();
        convoluter->Update();
    }

    // Read the correlation image (spectrogram)
    spectral::array result( static_cast<spectral::index>(nI),
                            static_cast<spectral::index>(nJ),
                            static_cast<spectral::index>(nK) );
    for(unsigned int k = 0; k < nK; ++k)
        for(unsigned int j = 0; j < nJ; ++j)
            for(unsigned int i = 0; i < nI; ++i){
                itk::Index<gridDim> index;
                index[0] = i;
                index[1] = nJ - 1 - j; // itkImage grid convention is different from GSLib's
                index[2] = nK - 1 - k;
                realType correlation = convoluter->GetOutput()->GetPixel( index );
                result( i, j, k ) = correlation;
            }

}
