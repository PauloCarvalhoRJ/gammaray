#include "gaborfilterdialog.h"
#include "ui_gaborfilterdialog.h"
#include "imagejockey/ijabstractvariable.h"
#include "imagejockey/ijabstractcartesiangrid.h"
#include "imagejockey/widgets/ijgridviewerwidget.h"
#include "imagejockey/widgets/ijquick3dviewer.h"
#include "spectral/spectral.h"
#include "imagejockey/svd/svdfactor.h"
#include <itkGaborImageSource.h>
#include <itkConvolutionImageFilter.h>
#include <itkGaussianInterpolateImageFunction.h>
#include <itkEuler3DTransform.h>
#include <itkResampleImageFilter.h>
#include <itkImageFileWriter.hxx>
#include <itkPNGImageIOFactory.h>
#include <itkCastImageFilter.h>
#include <itkRescaleIntensityImageFilter.hxx>

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
    double azimuth = ui->txtAzimuth->text().toDouble();
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

    //define the frequency schedule
    uint s0 = 1;
    uint s1 = s0 + ui->spinNumberOfFrequencySteps->value() - 1;
    double f0 = ui->txtInitialFrequency->text().toDouble();
    double f1 = ui->txtFinalFrequency->text().toDouble();

    //this lambda interpolates between the initial frequency (f0)
    //and final frequency(f1) logarithmically
    //
    // readable formula:
    //
    //    log(f1) - log(f0)       s1 - s0
    //  --------------------- = -----------
    //    log(f)  - log(f0)        s - s0
    //
    //  Where:
    //
    //  f: output interpolated frequency
    //  s: input step number
    //  s1 and s0: final and intial step numbers.
    //  f1 and f0: final and initial frequencies.
    //
    std::function<double (int)> f = [ s0, s1, f0, f1 ](int s)
                          { return std::pow(10.0,
                                  ( (s-s0)*(std::log10(f1)-std::log10(f0))/(s1-s0) ) + std::log10( f0 )
                                            ); };
    // This is the correlation cube (spectrogram)
    // I and J are the index of the input image
    // K is the index for each frequency (vertical)
    spectral::array spectrogram( static_cast<spectral::index>(nI),
                            static_cast<spectral::index>(nJ),
                            static_cast<spectral::index>( s1 - s0 ) );

    //TODO: performance: this loop could be parallelized
    for( uint step = s0; step <= s1; ++step ){
        double frequency = f( step );

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
            // if the image is 2D, the kernel doesn't need to be a cube (performance)
            uint nK_kernel = 255;
            if( nK == 1 )
                nK_kernel = 1;
            ImageType::RegionType::SizeType size; size[0] = 255; size[1] = 255; size[2] = nK_kernel;
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

        // Convolve the input image against the resampled gabor image kernel.
        typename ConvolutionFilterType::Pointer convoluter = ConvolutionFilterType::New();
        {
            convoluter->SetInput( inputImage );
            convoluter->SetKernelImage( resampler->GetOutput() );
            convoluter->NormalizeOn();
            convoluter->Update();
        }

        // Read the correlation image (spectrogram)
        for(unsigned int j = 0; j < nJ; ++j)
            for(unsigned int i = 0; i < nI; ++i) {
                    itk::Index<gridDim> index;
                    index[0] = i;
                    index[1] = nJ - 1 - j; // itkImage grid convention is different from GSLib's
                    index[2] = nK - 1 - 0 /*k*/;
                    realType correlation = convoluter->GetOutput()->GetPixel( index );
                    spectrogram( i, j, step - s0 ) = correlation;
            }
    }

    //Debug the spectrogram cube
    IJQuick3DViewer* ijqv = new IJQuick3DViewer();
    ijqv->display( spectrogram, spectrogram.min(), spectrogram.max() );
    ijqv->show();
}
