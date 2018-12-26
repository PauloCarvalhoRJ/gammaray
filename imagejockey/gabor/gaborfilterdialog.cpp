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
#include "imagejockey/gabor/gaborscandialog.h"
#include "imagejockey/gabor/gaborutils.h"
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

    m_kernelViewer1 = new IJQuick3DViewer();
    ui->frmKernelDisplay1->layout()->addWidget( m_kernelViewer1 );
    m_kernelViewer1->hideDismissButton();
    m_kernelViewer2 = new IJQuick3DViewer();
    m_kernelViewer2->hideDismissButton();
    ui->frmKernelDisplay2->layout()->addWidget( m_kernelViewer2 );
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
        //In this case, the input values are transformed to their absolute values.
        vtkSmartPointer<vtkImageData> spectrogramGrid = vtkSmartPointer<vtkImageData>::New();
        ImageJockeyUtils::makeVTKImageDataFromSpectralArray( spectrogramGrid, *m_spectrogram,
                                                             [] (double x) { return std::abs( x ); } );

        //make a cell-centered VTK grid from the corner-point values
        //with the same grid specs of the input 2D grid.
        int* extent = spectrogramGrid->GetExtent();
        vtkImageData* spectrogramGridAsCellCentered = vtkImageData::New();
        spectrogramGridAsCellCentered->SetSpacing ( m_inputGrid->getCellSizeI(),
                                                    m_inputGrid->getCellSizeJ(),
                                                    1);
        spectrogramGridAsCellCentered->SetOrigin ( m_inputGrid->getOriginX() - m_inputGrid->getCellSizeI()/2,
                                                   m_inputGrid->getOriginY() - m_inputGrid->getCellSizeJ()/2,
                                                  -0.5);
        spectrogramGridAsCellCentered->SetExtent( extent[0], extent[1]+1,
                                                  extent[2], extent[3]+1,
                                                  extent[4], extent[5]+1 );
        spectrogramGridAsCellCentered->GetCellData()->SetScalars (
                    spectrogramGrid->GetPointData()->GetScalars());

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
            spectrogramGridAsCellCentered->GetCellData()->AddArray( visibility );
        }

        // configure a thresholding object to make cells below cut-off invisible
        vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
        {
            threshold->SetInputData( spectrogramGridAsCellCentered );
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
                //the 5th parameter is transparency (0.0 == fully transparent, 1.0 == full opaque)
                lut->SetTableValue( i, rgb[0], rgb[1], rgb[2] /*, i/(double)tableSize*/ );
            }
            lut->SetRampToLinear();
            lut->Build();
        }

        //Create a VTK mapper for the VTK grid
        vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper->SetInputConnection( threshold->GetOutputPort() );
        mapper->SetLookupTable( lut );
        mapper->SetScalarRange( colorScaleMin, colorScaleMax );

        //Configure the spectrogram actor.
        spectrogramActor->GetProperty()->SetInterpolationToFlat();
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

void GaborFilterDialog::onScan()
{
    GaborScanDialog* gsd = new GaborScanDialog( m_inputGrid,
                                                m_inputVariableIndex,
                                                ui->txtMeanMajorAxis->text().toDouble(),
                                                ui->txtMeanMinorAxis->text().toDouble(),
                                                ui->txtSigmaMajorAxis->text().toDouble(),
                                                ui->txtSigmaMinorAxis->text().toDouble(),
                                                ui->spinKernelSizeI->value(),
                                                ui->spinKernelSizeJ->value()
                                                );
    gsd->show();
}

void GaborFilterDialog::updateKernelDisplays()
{
    int kernelNI = ui->spinKernelSizeI->value();
    int kernelNJ = ui->spinKernelSizeJ->value();

    GaborUtils::ImageTypePtr kernelF0 = GaborUtils::createGaborKernel(
                                                ui->txtInitialFrequency->text().toDouble(),
                                                ui->txtAzimuth->text().toDouble(),
                                                ui->txtMeanMajorAxis->text().toDouble(),
                                                ui->txtMeanMinorAxis->text().toDouble(),
                                                ui->txtSigmaMajorAxis->text().toDouble(),
                                                ui->txtSigmaMinorAxis->text().toDouble(),
                                                kernelNI,
                                                kernelNJ
                                              );

    GaborUtils::ImageTypePtr kernelF1 = GaborUtils::createGaborKernel(
                                                ui->txtFinalFrequency->text().toDouble(),
                                                ui->txtAzimuth->text().toDouble(),
                                                ui->txtMeanMajorAxis->text().toDouble(),
                                                ui->txtMeanMinorAxis->text().toDouble(),
                                                ui->txtSigmaMajorAxis->text().toDouble(),
                                                ui->txtSigmaMinorAxis->text().toDouble(),
                                                kernelNI,
                                                kernelNJ
                                              );

    spectral::array kernelData1( static_cast<spectral::index>( kernelNI ),
                                 static_cast<spectral::index>( kernelNJ ) );
    spectral::array kernelData2( static_cast<spectral::index>( kernelNI ),
                                 static_cast<spectral::index>( kernelNJ ) );
    for(unsigned int j = 0; j < kernelNJ; ++j)
        for(unsigned int i = 0; i < kernelNI; ++i) {
                itk::Index<GaborUtils::gridDim> index;
                index[0] = i;
                index[1] = kernelNJ - 1 - j; // itkImage grid convention is different from GSLib's
                kernelData1( i, j ) = kernelF0->GetPixel( index );
                kernelData2( i, j ) = kernelF1->GetPixel( index );
        }

    double colorScaleMin1 = kernelData1.min();
    double colorScaleMax1 = kernelData1.max();
    double colorScaleMin2 = kernelData2.min();
    double colorScaleMax2 = kernelData2.max();

    m_kernelViewer1->display( kernelData1, colorScaleMin1, colorScaleMax1 );
    m_kernelViewer2->display( kernelData2, colorScaleMin2, colorScaleMax2 );
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

    ///----------------------------user-defined Gabor parameters------------------------------
    double azimuth = ui->txtAzimuth->text().toDouble();
    ///---------------------------------------------------------------------------------------

    //get grid geometry
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

    GaborUtils::ImageTypePtr inputAsITK = GaborUtils::createITKImageFromCartesianGrid( *m_inputGrid,
                                                                                       m_inputVariableIndex);

    //////////////////////////////////
    QProgressDialog progressDialog;
    progressDialog.show();
    progressDialog.setLabelText("Performing convolutions...");
    progressDialog.setMinimum( s0 );
    progressDialog.setMaximum( s1 );
    progressDialog.show();
    /////////////////////////////////

    for( uint step = s0; step < s1; ++step ){
        double frequency = f( step );

        //update the progress window
        progressDialog.setValue( step );
        QApplication::processEvents();

        GaborUtils::ImageTypePtr response = GaborUtils::computeGaborResponse( frequency,
                                                                              azimuth,
                                                                              ui->txtMeanMajorAxis->text().toDouble(),
                                                                              ui->txtMeanMinorAxis->text().toDouble(),
                                                                              ui->txtSigmaMajorAxis->text().toDouble(),
                                                                              ui->txtSigmaMinorAxis->text().toDouble(),
                                                                              ui->spinKernelSizeI->value(),
                                                                              ui->spinKernelSizeJ->value(),
                                                                              inputAsITK );

        // Read the response image to build the spectrogram
        for(unsigned int j = 0; j < nJ; ++j)
            for(unsigned int i = 0; i < nI; ++i) {
                    itk::Index<GaborUtils::gridDim> index;
                    index[0] = i;
                    index[1] = nJ - 1 - j; // itkImage grid convention is different from GSLib's
                    GaborUtils::realType correlation = response->GetPixel( index );
                    (*m_spectrogram)( i, j, step - s0 ) = correlation;
            }
    }

    // set the color scale form fields to suitable initial values
    double absOfMin = std::abs( m_spectrogram->min() );
    double absOfMax = std::abs( m_spectrogram->max() );
    ui->txtColorScaleMin->setText( "0.0" );
    ui->txtColorScaleMax->setText( QString::number( std::max( absOfMax, absOfMin ) ) );

    //Update the spectrogram analyser.
    updateDisplay();

}
