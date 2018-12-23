//----------Since we're not building with CMake, we need to init the VTK modules------------------
//--------------linking with the VTK libraries is often not enough--------------------------------
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2) // VTK was built with vtkRenderingOpenGL2
VTK_MODULE_INIT(vtkInteractionStyle)
VTK_MODULE_INIT(vtkRenderingFreeType)
//------------------------------------------------------------------------------------------------
#include "gaborfilterdialog.h"
#include "ui_gaborfilterdialog.h"
#include "imagejockey/ijabstractvariable.h"
#include "imagejockey/ijabstractcartesiangrid.h"
#include "imagejockey/widgets/ijgridviewerwidget.h"
#include "imagejockey/widgets/ijquick3dviewer.h"
#include "imagejockey/svd/svdfactor.h"
#include "imagejockey/imagejockeyutils.h"
#include <itkGaborImageSource.h>
#include <itkConvolutionImageFilter.h>
#include <itkGaussianInterpolateImageFunction.h>
#include <itkEuler2DTransform.h>
#include <itkResampleImageFilter.h>
#include <itkImageFileWriter.hxx>
#include <itkPNGImageIOFactory.h>
#include <itkCastImageFilter.h>
#include <itkRescaleIntensityImageFilter.hxx>
#include <QProgressDialog>
#include <QVTKOpenGLWidget.h>
#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkRenderer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkPolyDataMapper.h>
#include <vtkSphereSource.h>
#include <vtkDataSetMapper.h>
#include <vtkImageData.h>
#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>
#include <vtkProperty.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkPointData.h>
#include <vtkScalarBarActor.h>
#include <vtkThreshold.h>
#include <vtkCellData.h>

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

    this->setWindowTitle( "Gabor Transform Dialog" );

    ///-------------------setup the 3D viewer-------------------
    _vtkwidget = new QVTKOpenGLWidget();

    _renderer = vtkSmartPointer<vtkRenderer>::New();

    // enable antialiasing
    _renderer->SetUseFXAA( true );

    _vtkwidget->SetRenderWindow(vtkGenericOpenGLRenderWindow::New());
    _vtkwidget->GetRenderWindow()->AddRenderer(_renderer);
    _vtkwidget->setFocusPolicy(Qt::StrongFocus);

    //----------------------adding the orientation axes-------------------------
    vtkSmartPointer<vtkAxesActor> axes = vtkSmartPointer<vtkAxesActor>::New();
    _vtkAxesWidget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
    _vtkAxesWidget->SetOutlineColor(0.9300, 0.5700, 0.1300);
    _vtkAxesWidget->SetOrientationMarker(axes);
    _vtkAxesWidget->SetInteractor(_vtkwidget->GetRenderWindow()->GetInteractor());
    _vtkAxesWidget->SetViewport(0.0, 0.0, 0.2, 0.2);
    _vtkAxesWidget->SetEnabled(1);
    _vtkAxesWidget->InteractiveOn();

    //set the background to a shade of gray
    _renderer->SetBackground(0.5, 0.5, 0.5);

    // adjusts view so everything fits in the screen
    _renderer->ResetCamera();

    // add the VTK widget the layout
    ui->frm3DDisplay->layout()->addWidget( _vtkwidget );
    //////////////////////////////////////////////////////////////
}

GaborFilterDialog::~GaborFilterDialog()
{
    delete ui;
}

void GaborFilterDialog::updateDisplay()
{
    if( ! m_spectrogram )
        return;

    //Clear current display
    clearDisplay();

    /////--------------------code to render the spectrogram cube-----------------------
    vtkSmartPointer<vtkActor> spectrogramActor = vtkSmartPointer<vtkActor>::New();
    vtkSmartPointer<vtkScalarBarActor> scalarBarActor = vtkSmartPointer<vtkScalarBarActor>::New();
    {
        //Get user settings.
        double colorScaleMin = ui->txtColorScaleMin->text().toDouble();
        double colorScaleMax = ui->txtColorScaleMax->text().toDouble();
        double thresholdValue = ui->txtThreshold->text().toDouble();

        //Convert the spectrogram cube into a corresponding VTK object.
        vtkSmartPointer<vtkImageData> spectrogramGrid = vtkSmartPointer<vtkImageData>::New();
        ImageJockeyUtils::makeVTKImageDataFromSpectralArray( spectrogramGrid, *m_spectrogram,
                                                             [] (double x) { return std::abs( x ); } );

        //create a visibility array. Cells with visibility >= 1 will be
        //visible, and < 1 will be invisible.
        vtkSmartPointer<vtkIntArray> visibility = vtkSmartPointer<vtkIntArray>::New();
        {
            visibility->SetNumberOfComponents(1);
            visibility->SetName("Visibility");
            int* extent = spectrogramGrid->GetExtent();
            int nI = extent[1] - extent[0];
            int nJ = extent[3] - extent[2];
            int nK = extent[5] - extent[4];
            visibility->Allocate( nI * nJ * nK );
            for (int k = extent[4]; k <= extent[5]; ++k){
                for (int j = extent[2]; j <= extent[3]; ++j){
                    for (int i = extent[0]; i <= extent[1]; ++i){
                        double* value = static_cast<double*>(spectrogramGrid->GetScalarPointer(i,j,k));
                        if ( value[0] < thresholdValue )
                            visibility->InsertNextValue( 0 );
                        else
                            visibility->InsertNextValue( 1 );
                    }
                }
            }
            spectrogramGrid->GetCellData()->AddArray( visibility );
        }

        // configure a thresholding object to make cells below cut-off invisible
        vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
        {
            threshold->SetInputData( spectrogramGrid );
            threshold->ThresholdByUpper(1); // Criterion is cells whose scalars are greater or equal to threshold.
            threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Visibility");
            threshold->Update();
        }

        //Create a color table
        vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
        {
            size_t tableSize = 32; //number of shades
            //create a color interpolator object (grayscale)
            vtkSmartPointer<vtkColorTransferFunction> ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
            ctf->SetColorSpaceToRGB();
            ctf->AddRGBPoint(0.00, 0.000, 0.000, 0.000);
            ctf->AddRGBPoint(1.00, 1.000, 1.000, 1.000);
            //configure the color table object
            lut->SetTableRange(colorScaleMin, colorScaleMax);
            lut->SetNumberOfTableValues(tableSize);
            for(size_t i = 0; i < tableSize; ++i)
            {
                double *rgb;
                rgb = ctf->GetColor(static_cast<double>(i)/tableSize);
                lut->SetTableValue(i, rgb[0], rgb[1], rgb[2]);
            }
            lut->SetRampToLinear();
            lut->Build();
        }

        //Create a VTK mapper for the VTK grid
        vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper->SetInputConnection( threshold->GetOutputPort() );
        mapper->SetLookupTable( lut );
        mapper->SetScalarRange( colorScaleMin, colorScaleMax );

        //Create the scene actor.
        spectrogramActor->SetMapper( mapper );

        //Confgure the correlation values scale bar
        scalarBarActor->SetLookupTable( lut );
        scalarBarActor->SetTitle("correlation");
        scalarBarActor->SetNumberOfLabels( 4 );
    }

    /////-----------------code to render the input grid (aid in interpretation)-------------------
    vtkSmartPointer<vtkActor> gridActor = vtkSmartPointer<vtkActor>::New();
    {
        double colorScaleMin = m_inputGrid->getMin( m_inputVariableIndex );
        double colorScaleMax = m_inputGrid->getMax( m_inputVariableIndex );

        //Convert the data grid into a corresponding VTK object.
        vtkSmartPointer<vtkImageData> out = vtkSmartPointer<vtkImageData>::New();
        spectral::arrayPtr gridData( m_inputGrid->createSpectralArray( m_inputVariableIndex ) );
        ImageJockeyUtils::makeVTKImageDataFromSpectralArray( out, *gridData );

        //put the input grid a bit far from the spectrogram cube
        double* origin = out->GetOrigin();
        origin[2] -= 10.0;
        out->SetOrigin( origin );

        //Create a color table
        vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
        {
            size_t tableSize = 32; //number of shades
            //create a color interpolator object (classic rainbow scale)
            vtkSmartPointer<vtkColorTransferFunction> ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
            ctf->SetColorSpaceToRGB();
            ctf->AddRGBPoint(0.00, 0.000, 0.000, 1.000);
            ctf->AddRGBPoint(0.25, 0.000, 1.000, 1.000);
            ctf->AddRGBPoint(0.50, 0.000, 1.000, 0.000);
            ctf->AddRGBPoint(0.75, 1.000, 1.000, 0.000);
            ctf->AddRGBPoint(1.00, 1.000, 0.000, 0.000);
            //configure the color table object
            lut->SetTableRange(colorScaleMin, colorScaleMax);
            lut->SetNumberOfTableValues(tableSize);
            for(size_t i = 0; i < tableSize; ++i)
            {
                double *rgb;
                rgb = ctf->GetColor(static_cast<double>(i)/tableSize);
                lut->SetTableValue(i, rgb[0], rgb[1], rgb[2]);
            }
            lut->SetRampToLinear();
            lut->Build();
        }

        //Create a VTK mapper for the VTK grid
        vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper->SetInputData( out );
        mapper->SetLookupTable( lut );
        mapper->SetScalarRange( colorScaleMin, colorScaleMax );

        //Create the scene actor.
        gridActor->SetMapper( mapper );
    }

    //////////////////////////////////////////////////////////////////////////////////////////////

    //Update the graphics system.
    _renderer->AddActor( spectrogramActor );
    _currentActors.push_back( spectrogramActor );
    _renderer->AddActor2D( scalarBarActor );
    _scaleBarActor = scalarBarActor;
    _renderer->AddActor( gridActor );
    _currentActors.push_back( gridActor );
    _renderer->ResetCamera();
    _vtkwidget->GetRenderWindow()->Render();
}

void GaborFilterDialog::clearDisplay()
{
    std::vector< vtkSmartPointer<vtkActor> >::iterator it = _currentActors.begin();
    for( ; it != _currentActors.end(); ){ // erase() already increments the iterator.
        _renderer->RemoveActor( *it );
        it = _currentActors.erase( it );
    }
    _renderer->RemoveActor( _scaleBarActor );
}

void GaborFilterDialog::onPerformGaborFilter()
{
    //typedfs and other definitions
    const unsigned int gridDim = 2;
    typedef float realType;
    typedef itk::Image<realType, gridDim> ImageType;
    typedef itk::GaborImageSource<ImageType> GaborSourceType;
    typedef itk::GaussianInterpolateImageFunction<ImageType, realType> GaussianInterpolatorType;
    typedef itk::Euler2DTransform<realType> TransformType;
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
    double x0 = m_inputGrid->getOriginX();
    double y0 = m_inputGrid->getOriginY();
    unsigned int nI = m_inputGrid->getNI();
    unsigned int nJ = m_inputGrid->getNJ();

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
    m_spectrogram = spectral::arrayPtr( new spectral::array(
                            static_cast<spectral::index>(nI),
                            static_cast<spectral::index>(nJ),
                            static_cast<spectral::index>( s1 - s0 ) ) );

    //////////////////////////////////
    QProgressDialog progressDialog;
    progressDialog.show();
    progressDialog.setLabelText("Performing convolutions...");
    progressDialog.setMinimum( s0 );
    progressDialog.setMaximum( s1 );
    progressDialog.show();
    /////////////////////////////////

    //TODO: performance: this loop could be parallelized
    for( uint step = s0; step <= s1; ++step ){
        double frequency = f( step );

        //update the progress window
        progressDialog.setValue( step );
        QApplication::processEvents();

        // Construct the gabor image kernel.
        // The idea is that we construct a gabor image kernel and
        // downsample it to a smaller size for image convolution.
        GaborSourceType::Pointer gabor = GaborSourceType::New();
        {
            ImageType::SpacingType spacing; spacing[0] = 1.0; spacing[1] = 1.0;
            gabor->SetSpacing( spacing );
            ////////////////////
            ImageType::PointType origin; origin[0] = 0.0; origin[1] = 0.0;
            gabor->SetOrigin( origin );
            ////////////////////
            ImageType::RegionType::SizeType size; size[0] = 255; size[1] = 255;
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
            ImageType::SpacingType spacing; spacing[0] = dX; spacing[1] = dY;
            inputImage->SetSpacing( spacing );
            ////////////////////
            ImageType::PointType origin; origin[0] = x0; origin[1] = y0;
            inputImage->SetOrigin( origin );
            size[0] = nI;
            size[1] = nJ;
            ImageType::RegionType region(start, size);
            inputImage->SetRegions(region);
            inputImage->Allocate();
            inputImage->FillBuffer(0);
            for(unsigned int j = 0; j < nJ; ++j)
                for(unsigned int i = 0; i < nI; ++i){
                    double value = m_inputGrid->getData( m_inputVariableIndex, i, j, 0 );
                    itk::Index<gridDim> index;
                    index[0] = i;
                    index[1] = nJ - 1 - j; // itkImage grid convention is different from GSLib's
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
            transform->SetRotation( azimuth );
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
                    realType correlation = convoluter->GetOutput()->GetPixel( index );
                    (*m_spectrogram)( i, j, step - s0 ) = correlation;
            }
    }

    //Debug the spectrogram cube
//    IJQuick3DViewer* ijqv = new IJQuick3DViewer();
//    ijqv->display( *m_spectrogram, m_spectrogram->min(), m_spectrogram->max() );
//    ijqv->show();

    // set the color scale form fields to suitable initial values
    double absOfMin = std::abs( m_spectrogram->min() );
    double absOfMax = std::abs( m_spectrogram->max() );
    ui->txtColorScaleMin->setText( "0.0" );
    ui->txtColorScaleMax->setText( QString::number( std::max( absOfMax, absOfMin ) ) );

    //Update the spectrogram analyser.
    updateDisplay();

}
