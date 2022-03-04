//----------Since we're not building with CMake, we need to init the VTK modules------------------
//--------------linking with the VTK libraries is often not enough--------------------------------
#include <QInputDialog>
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2) // VTK was built with vtkRenderingOpenGL2
VTK_MODULE_INIT(vtkRenderingContextOpenGL2)//solves a "no override found..." runtime error
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
#include "imagejockey/gabor/gaborutils.h"
#include "imagejockey/paraviewscalarbar/vtkParaViewScalarBar.h"
#include "util.h"
#include <QProgressDialog>
#include <QVTKOpenGLNativeWidget.h>
#include <QMessageBox>
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
#include <vtkCubeAxesActor2D.h>
#include <vtkTextProperty.h>
#include <vtkDiscretizableColorTransferFunction.h>

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

    this->setWindowTitle( "Gabor Analysis Dialog" );

    //double the width/height of the some panels for UHD displays
    if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI ){
        ui->frmConfig->setMaximumWidth( 2 * ui->frmConfig->maximumWidth() );
        ui->frmKernelDisplay1->setMinimumHeight( 2 * ui->frmKernelDisplay1->minimumHeight() );
        ui->frmKernelDisplay2->setMinimumHeight( 2 * ui->frmKernelDisplay2->minimumHeight() );
    }

    ///-------------------setup the 3D viewer-------------------
    _vtkwidget = new QVTKOpenGLNativeWidget();

    _renderer = vtkSmartPointer<vtkRenderer>::New();

    // enable antialiasing
    _renderer->SetUseFXAA( true );

    _vtkwidget->setRenderWindow(vtkGenericOpenGLRenderWindow::New());
    _vtkwidget->renderWindow()->AddRenderer(_renderer);
    _vtkwidget->setFocusPolicy(Qt::StrongFocus);

    //----------------------adding the orientation axes-------------------------
    vtkSmartPointer<vtkAxesActor> axes = vtkSmartPointer<vtkAxesActor>::New();
    _vtkAxesWidget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
    _vtkAxesWidget->SetOutlineColor(0.9300, 0.5700, 0.1300);
    _vtkAxesWidget->SetOrientationMarker(axes);
    _vtkAxesWidget->SetInteractor(_vtkwidget->renderWindow()->GetInteractor());
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

    ui->spinKernelSizeI->setMaximum( GaborUtils::gaborKernelMaxNI );
    ui->spinKernelSizeJ->setMaximum( GaborUtils::gaborKernelMaxNJ );

    ui->lblFeatureSizeEquiv->setStyleSheet( "background-color: black; border-radius: 10px; color: #00FF00;" );

    onUserEditedAFrequency("");
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

    //define the frequency schedule
    uint s0 = 1;
    uint s1 = s0 + ui->spinNumberOfFrequencySteps->value() - 1;
    double f0 = ui->txtInitialFrequency->text().toDouble();
    double f1 = ui->txtFinalFrequency->text().toDouble();


    //get the user-entered topological frequencies
    // the frequencies are topological (that is, inverse of cell counts)
    // because the Gabor transform involves convolutions, which are cell-centered
    // operations in grids (they ignore geometry).
    //get the user-entered kernel sizes
    int kernelNI = ui->spinKernelSizeI->value();
    int kernelNJ = ui->spinKernelSizeJ->value();
    //get the system-defined maximum kernel resolution
    int maxNI = GaborUtils::gaborKernelMaxNI;
    int maxNJ = GaborUtils::gaborKernelMaxNJ;
    //convert the user-entered topological frequencies as cell counts.
    double nCellsIInitial = kernelNI * ( 1 / f0 ) / maxNI;
    double nCellsJInitial = kernelNJ * ( 1 / f0 ) / maxNJ;
    double nCellsIFinal = kernelNI * ( 1 / f1 ) / maxNI;
    double nCellsJFinal = kernelNJ * ( 1 / f1 ) / maxNJ;
    //convert cell counts to real-world lengths
    double featureSizeXInitial = nCellsIInitial * m_inputGrid->getCellSizeI();
    double featureSizeYInitial = nCellsJInitial * m_inputGrid->getCellSizeJ();
    double featureSizeXFinal = nCellsIFinal * m_inputGrid->getCellSizeI();
    double featureSizeYFinal = nCellsJFinal * m_inputGrid->getCellSizeJ();
    //get resultant feature sizes
    double featureSizeInitial = ( featureSizeXInitial + featureSizeYInitial ) / 2;
    double featureSizeFinal = ( featureSizeXFinal + featureSizeYFinal ) / 2;
    //get the feature size step size (intial size is greater because initial frequency is lower)
    double dFeatureSize = ( featureSizeInitial - featureSizeFinal ) / ( s1 - s0 ) ;


    /////--------------------code to render the spectrogram cube-----------------------
    vtkSmartPointer<vtkActor> spectrogramActor = vtkSmartPointer<vtkActor>::New();
    {
        //Get user settings.
        double colorScaleMin = ui->txtColorScaleMin->text().toDouble();
        double colorScaleMax = ui->txtColorScaleMax->text().toDouble();
        double thresholdMinValue = ui->txtThresholdMin->text().toDouble();
        double thresholdMaxValue = ui->txtThreshold->text().toDouble();

        //Convert the spectrogram cube into a corresponding VTK object.
        //In this case, the input values are transformed to their absolute values.
        vtkSmartPointer<vtkImageData> spectrogramGrid = vtkSmartPointer<vtkImageData>::New();
        ImageJockeyUtils::makeVTKImageDataFromSpectralArray( spectrogramGrid, *m_spectrogram,
                                                             [] (double x) { return std::abs( x ); } );
//        ImageJockeyUtils::makeVTKImageDataFromSpectralArray( spectrogramGrid, *m_spectrogram );

        //make a cell-centered VTK grid from the corner-point values
        //with the same grid specs of the input 2D grid.
        int* extent = spectrogramGrid->GetExtent();
        vtkImageData* spectrogramGridAsCellCentered = vtkImageData::New();
        spectrogramGridAsCellCentered->SetSpacing ( m_inputGrid->getCellSizeI(),
                                                    m_inputGrid->getCellSizeJ(),
                                                    dFeatureSize);
        spectrogramGridAsCellCentered->SetOrigin ( m_inputGrid->getOriginX() - m_inputGrid->getCellSizeI()/2,
                                                   m_inputGrid->getOriginY() - m_inputGrid->getCellSizeJ()/2,
                                                   featureSizeFinal - 0.5 * dFeatureSize );
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
                        if ( value[0] >= thresholdMinValue && value[0] <= thresholdMaxValue )
                            visibility->InsertNextValue( 1 );
                        else
                            visibility->InsertNextValue( 0 );
                    }
                }
            }
            spectrogramGridAsCellCentered->GetCellData()->AddArray( visibility );
        }

        // configure a thresholding object to make cells below cut-off invisible
        vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
        {
            threshold->SetInputData( spectrogramGridAsCellCentered );
            threshold->SetUpperThreshold(1); // Criterion is cells whose scalars are greater or equal to threshold.
            threshold->SetThresholdFunction( vtkThreshold::THRESHOLD_UPPER );
            threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Visibility");
            threshold->Update();
        }

        //create a color interpolator object (black->yellow->red->white)
        vtkSmartPointer<vtkDiscretizableColorTransferFunction> ctf = vtkSmartPointer<vtkDiscretizableColorTransferFunction>::New();
        {
            ctf->SetColorSpaceToRGB();
            double delta = colorScaleMax - colorScaleMin;
            ctf->AddRGBPoint(colorScaleMin, 0.000, 0.000, 0.000);
            ctf->AddRGBPoint(colorScaleMin + 0.33 * delta, 1.000, 1.000, 0.000);
            ctf->AddRGBPoint(colorScaleMin + 0.66 * delta, 1.000, 0.000, 0.000);
            ctf->AddRGBPoint(colorScaleMax, 1.000, 1.000, 1.000);
        }

        //Create a VTK mapper for the VTK grid
        vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
        mapper->SetInputConnection( threshold->GetOutputPort() );
        mapper->SetLookupTable( ctf );
        mapper->SetScalarRange( colorScaleMin, colorScaleMax );

        //Configure the spectrogram actor.
        spectrogramActor->GetProperty()->SetInterpolationToFlat();
        spectrogramActor->SetMapper( mapper );

        //Configure the correlation values scale bar
        _scalarBar->SetLookupTable( ctf );
        _scalarBar->SetTitle("amplitude");
        //scalarBar->SetNumberOfLabels( 4 );
        _scalarBar->SetRenderer( _renderer );
        _scalarBar->SetInteractor( _vtkwidget->renderWindow()->GetInteractor() );

        // Create a text style for the cube axes
        vtkSmartPointer<vtkTextProperty> tprop = vtkSmartPointer<vtkTextProperty>::New();
        tprop->SetColor(1, 1, 1);
        tprop->ShadowOn();
        tprop->SetFontSize(20);

        // add scaling to the edges of the cube.
        _axes = vtkSmartPointer<vtkCubeAxesActor2D>::New();
        _axes->SetViewProp( spectrogramActor.GetPointer() );
        _axes->SetCamera( _renderer->GetActiveCamera() );
        _axes->SetLabelFormat("%6.4g");
        _axes->SetFlyModeToClosestTriad();
        _axes->ScalingOff();
        _axes->SetNumberOfLabels( 5 );
        _axes->SetZLabel("Feature Size");
        _axes->SetAxisTitleTextProperty( tprop.GetPointer() );
        _axes->SetAxisLabelTextProperty( tprop.GetPointer() );
        _renderer->AddViewProp( _axes.GetPointer() );
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
    _renderer->AddActor( gridActor );
    _currentActors.push_back( gridActor );
    _renderer->ResetCamera();
    _vtkwidget->renderWindow()->Render();
}

void GaborFilterDialog::onScan()
{
    m_gsd = new GaborScanDialog( m_inputGrid,
                                 m_inputVariableIndex,
                                 ui->txtMeanMajorAxis->text().toDouble(),
                                 ui->txtMeanMinorAxis->text().toDouble(),
                                 ui->txtSigmaMajorAxis->text().toDouble(),
                                 ui->txtSigmaMinorAxis->text().toDouble(),
                                 ui->spinKernelSizeI->value(),
                                 ui->spinKernelSizeJ->value(),
                                 this
                                );
    connect( m_gsd, SIGNAL(frequencyAzimuthSelectionUpdated(GaborFrequencyAzimuthSelections)),
             this,  SLOT(onFreqAzSelectionsUpdated(GaborFrequencyAzimuthSelections)) );
    m_gsd->show();
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
                                                kernelNJ,
                                                false
                                              );

    GaborUtils::ImageTypePtr kernelF1 = GaborUtils::createGaborKernel(
                                                ui->txtFinalFrequency->text().toDouble(),
                                                ui->txtAzimuth->text().toDouble(),
                                                ui->txtMeanMajorAxis->text().toDouble(),
                                                ui->txtMeanMinorAxis->text().toDouble(),
                                                ui->txtSigmaMajorAxis->text().toDouble(),
                                                ui->txtSigmaMinorAxis->text().toDouble(),
                                                kernelNI,
                                                kernelNJ,
                                                false
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

void GaborFilterDialog::onFreqAzSelectionsUpdated(const GaborFrequencyAzimuthSelections &freqAzSelections)
{
    m_freqAzSelections = freqAzSelections;
}

void GaborFilterDialog::onUserEditedAFrequency(QString freqValue)
{
    Q_UNUSED( freqValue )
    //get the user-entered topological frequencies
    // the frequencies are topological (that is, inverse of cell counts)
    // because the Gabor transform involves convolutions, which are cell-centered
    // operations in grids (they ignore geometry).
    double topologicalFrequencyInitial = ui->txtInitialFrequency->text().toDouble();
    double topologicalFrequencyFinal = ui->txtFinalFrequency->text().toDouble();
    //get the user-entered kernel sizes
    int kernelNI = ui->spinKernelSizeI->value();
    int kernelNJ = ui->spinKernelSizeJ->value();
    //get the system-defined maximum kernel resolution
    int maxNI = GaborUtils::gaborKernelMaxNI;
    int maxNJ = GaborUtils::gaborKernelMaxNJ;
    //convert the user-entered topological frequencies as cell counts.
    double nCellsIInitial = kernelNI * ( 1 / topologicalFrequencyInitial) / maxNI;
    double nCellsJInitial = kernelNJ * ( 1 / topologicalFrequencyInitial) / maxNJ;
    double nCellsIFinal = kernelNI * ( 1 / topologicalFrequencyFinal) / maxNI;
    double nCellsJFinal = kernelNJ * ( 1 / topologicalFrequencyFinal) / maxNJ;
    //convert cell counts to real-world lengths
    double featureSizeXInitial = nCellsIInitial * m_inputGrid->getCellSizeI();
    double featureSizeYInitial = nCellsJInitial * m_inputGrid->getCellSizeJ();
    double featureSizeXFinal = nCellsIFinal * m_inputGrid->getCellSizeI();
    double featureSizeYFinal = nCellsJFinal * m_inputGrid->getCellSizeJ();

    //display the feature sizes
    //lower frequency (initial) translates to larger features and vice-versa.
    QString textForLabel = "<html><head/><body><b>&nbsp;&nbsp;&nbsp;Feature sizes equivalency:</b><br>";
    textForLabel += "&nbsp;&nbsp;&nbsp;max. (X direction) = " + QString::number(featureSizeXInitial) + "<br>";
    textForLabel += "&nbsp;&nbsp;&nbsp;max. (Y direction) = " + QString::number(featureSizeYInitial) + "<br>";
    textForLabel += "&nbsp;&nbsp;&nbsp;min. (X direction) = " + QString::number(featureSizeXFinal) + "<br>";
    textForLabel += "&nbsp;&nbsp;&nbsp;min. (Y direction) = " + QString::number(featureSizeYFinal) + "<br>";
    textForLabel += "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;sizes are in length units.</body></html>";

    ui->lblFeatureSizeEquiv->setText( textForLabel );
}

void GaborFilterDialog::onPreviewFilteredResult()
{
    if( m_freqAzSelections.empty() ){
        QMessageBox::critical( this, "Error", "Please, add at least one min./max. frequency/azimuth range in the Gabor scan dialog (the \"scan\" button).");
        return;
    }

    performFiltering();

    debugGrid( m_filteredResult );
}

void GaborFilterDialog::onSaveFilteredResult()
{
    if( m_freqAzSelections.empty() ){
        QMessageBox::critical( this, "Error", "Please, add at least one min./max. frequency/azimuth range in the Gabor scan dialog (the \"scan\" button).");
        return;
    }

    performFiltering();

    QString proposed_name = m_inputGrid->getVariableByIndex( m_inputVariableIndex )->getVariableName();
    proposed_name += "_filtered";

    //open file rename dialog
    bool ok;
    QString new_name = QInputDialog::getText(this, "Name the variable",
                                             "New variable with filtered results:", QLineEdit::Normal,
                                             proposed_name, &ok);
    if( ! ok )
        return;

    m_inputGrid->appendAsNewVariable( new_name , m_filteredResult );
    m_inputGrid->saveData();

}

void GaborFilterDialog::clearDisplay()
{
    std::vector< vtkSmartPointer<vtkActor> >::iterator it = _currentActors.begin();
    for( ; it != _currentActors.end(); ){ // erase() already increments the iterator.
        _renderer->RemoveActor( *it );
        it = _currentActors.erase( it );
    }
    _renderer->RemoveViewProp( _axes.GetPointer() );
}

void GaborFilterDialog::performFiltering()
{
    //get input grid sizes
    spectral::index nI = m_inputGrid->getNI();
    spectral::index nJ = m_inputGrid->getNJ();

    //compute the FFT of the input data in polar form (magnitude and phase).
    spectral::complex_array inputFFTpolar;
    {
        spectral::complex_array inputFFT;
        spectral::arrayPtr inputData( m_inputGrid->createSpectralArray( m_inputVariableIndex ) );
        spectral::foward( inputFFT, *inputData );
        inputFFTpolar = spectral::to_polar_form( inputFFT );
    }

    //this map is to sum up the FFT amplitudes of each Gabor kernel resulting from the
    //azimuth and frequency selection set by the user
    spectral::array kernelFFTamplUnitizedSum( nI, nJ, 1, 0.0 );

    double rangeDiv = 20.0;

    //////////////////////////////////
    QProgressDialog progressDialog;
    progressDialog.show();
    progressDialog.setLabelText("Performing back-transforms...");
    progressDialog.setMinimum( 0 );
    progressDialog.setMaximum( m_freqAzSelections.size() * rangeDiv * rangeDiv );
    progressDialog.show();
    /////////////////////////////////

    int progressCounter = 0;
    for( const GaborFrequencyAzimuthSelection& gFAzSel : m_freqAzSelections ){
        double dAz = ( gFAzSel.maxAz - gFAzSel.minAz ) / rangeDiv;
        double dF = ( gFAzSel.maxF - gFAzSel.minF ) / rangeDiv;
        for( double azimuth = gFAzSel.minAz; azimuth <= gFAzSel.maxAz; azimuth += dAz )
            for( double frequency = gFAzSel.minF; frequency <= gFAzSel.maxF; frequency += dF ){

                progressDialog.setValue( progressCounter );
                QApplication::processEvents();

                //compute the unitized (min. = 0.0, max. = 1.0) amplitude of FFT of the Gabor kernel of a given azimuth and frequency.
                spectral::array kernelFFTamplUnitized;
                {
                    //make the kernel
                    GaborUtils::ImageTypePtr kernel = GaborUtils::createGaborKernel(frequency,
                                                                                    azimuth,
                                                                                    ui->txtMeanMajorAxis->text().toDouble(),
                                                                                    ui->txtMeanMinorAxis->text().toDouble(),
                                                                                    ui->txtSigmaMajorAxis->text().toDouble(),
                                                                                    ui->txtSigmaMinorAxis->text().toDouble(),
                                                                                    ui->spinKernelSizeI->value(),
                                                                                    ui->spinKernelSizeJ->value(),
                                                                                    false );

                    //convert it to a spectral:: array
                    spectral::array kernelS = GaborUtils::convertITKImageToSpectralArray( *kernel );

                    //makes it compatible with inner multiplication with the input grid (same size)
                    spectral::array kernelPadded = spectral::project( kernelS, nI, nJ, 1 );

                    //compute the FFT
                    spectral::complex_array kernelFFT;
                    spectral::foward( kernelFFT, kernelPadded );

                    //put the complex numbers into polar form
                    spectral::complex_array kernelFFTpolar = spectral::to_polar_form( kernelFFT );

                    //get the amplitde values.
                    spectral::array kernelFFTampl = spectral::real( kernelFFTpolar );

                    //rescale the amplitudes to 0.0-1.0
                    kernelFFTamplUnitized = kernelFFTampl / kernelFFTampl.max();

                    //add this kernel's contribution
                    kernelFFTamplUnitizedSum.updateMax( kernelFFTamplUnitized );

                    ++progressCounter;
                }
            }
    }

    //unitize the sum of kernel FFT amplitudes
    //kernelFFTamplUnitizedSum = kernelFFTamplUnitizedSum / kernelFFTamplUnitizedSum.max();

    //multiply the magnitude of the input  with the magnitude of the kernel, effectivelly separating the desired frequency/azimuth
    spectral::array filteredAmplitude = spectral::hadamard( spectral::real( inputFFTpolar ),
                                                            kernelFFTamplUnitizedSum );

    //make a new FFT field by combinind the filtered amplitudes with the untouched phase field of the input
    spectral::complex_array filteredFFTpolar = spectral::to_complex_array(
                               filteredAmplitude ,
                               spectral::imag( inputFFTpolar) );

    //convert the filtered FFT field to Cartesian form (real and imaginary parts)
    spectral::complex_array filteredFFT = spectral::to_rectangular_form( filteredFFTpolar );


    //performs inverse FFT to get the filtered result in spatial domain.
    m_filteredResult = spectral::array( nI, nJ, 1, 0.0 );
    spectral::backward( m_filteredResult, filteredFFT );

    //fftw3 requires that the result be divided by the number of grid cells to get the correct scale
    m_filteredResult = m_filteredResult / ( nI * nJ );
}

void GaborFilterDialog::debugGrid(const spectral::array &grid)
{
    spectral::array result ( grid );
    SVDFactor* gridSVD = new SVDFactor( std::move(result), 1, 0.42,
                                     m_inputGrid->getOriginX(),
                                     m_inputGrid->getOriginY(),
                                     0.0,
                                     m_inputGrid->getCellSizeI(),
                                     m_inputGrid->getCellSizeJ(),
                                     1.0, 0.0 );
    IJGridViewerWidget* ijgv = new IJGridViewerWidget( true, false, true );
    ijgv->setFactor( gridSVD );
    ijgv->show();
}

void GaborFilterDialog::onPerformGaborFilter()
{

    ///----------------------------user-defined Gabor parameters------------------------------
    bool singleAzimuth = ui->chkSingleAzimuth->isChecked();
    double azimuthFinalIfNotSingle = ui->txtAzFinal->text().toDouble();
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
    //and final frequency(f1) logarithmically (concavity upwards)
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
//    std::function<double (int)> f = [ s0, s1, f0, f1 ](int s)
//                          { return std::pow(10.0,
//                                  ( (s-s0)*(std::log10(f1)-std::log10(f0))/(s1-s0) ) + std::log10( f0 )
//                                            ); };

    // This is the correlation cube (spectrogram)
    // I and J are the index of the input image
    // K is the index for each frequency (vertical)
    m_spectrogram = spectral::arrayPtr( new spectral::array(
                            static_cast<spectral::index>(nI),
                            static_cast<spectral::index>(nJ),
                            static_cast<spectral::index>( s1 - s0 ) ) );

    spectral::arrayPtr inputAsArray( m_inputGrid->createSpectralArray( m_inputVariableIndex ) );

//    GaborUtils::ImageTypePtr inputImage =
//            GaborUtils::createITKImageFromCartesianGrid( *m_inputGrid, m_inputVariableIndex );

    //////////////////////////////////
    QProgressDialog progressDialog;
    progressDialog.show();
    progressDialog.setLabelText("Performing convolutions...");
    progressDialog.setMinimum( s0 );
    progressDialog.setMaximum( s1 );
    progressDialog.show();
    /////////////////////////////////

    //for each frequency in the schedule
    double df = ( f1 - f0 ) / ( s1 - s0 ) ;
    for( uint step = s0; step < s1; ++step ){

        //get the current frequency of the schedule
        //double frequency = f( step );
        double frequency = f0 + (step - s0) * df;

        //update the progress window
        progressDialog.setValue( step );
        QApplication::processEvents();

        double azimuth = ui->txtAzimuth->text().toDouble();

        while( true ){
            //get the response of the Gabor filter (real part)
            GaborUtils::ImageTypePtr responseRealPart = GaborUtils::computeGaborResponse( frequency,
                                                                                  azimuth,
                                                                                  ui->txtMeanMajorAxis->text().toDouble(),
                                                                                  ui->txtMeanMinorAxis->text().toDouble(),
                                                                                  ui->txtSigmaMajorAxis->text().toDouble(),
                                                                                  ui->txtSigmaMinorAxis->text().toDouble(),
                                                                                  ui->spinKernelSizeI->value(),
                                                                                  ui->spinKernelSizeJ->value(),
                                                                                  *inputAsArray,
                                                                                  false );
            //get the response of the Gabor filter (imaginary part)
            GaborUtils::ImageTypePtr responseImaginaryPart = GaborUtils::computeGaborResponse( frequency,
                                                                                  azimuth,
                                                                                  ui->txtMeanMajorAxis->text().toDouble(),
                                                                                  ui->txtMeanMinorAxis->text().toDouble(),
                                                                                  ui->txtSigmaMajorAxis->text().toDouble(),
                                                                                  ui->txtSigmaMinorAxis->text().toDouble(),
                                                                                  ui->spinKernelSizeI->value(),
                                                                                  ui->spinKernelSizeJ->value(),
                                                                                  *inputAsArray,
                                                                                  true );

            // Read the response image to build the amplitude spectrogram
            for(unsigned int j = 0; j < nJ; ++j)
                for(unsigned int i = 0; i < nI; ++i) {
                        itk::Index<GaborUtils::gridDim> index;
                        index[0] = i;
                        index[1] = nJ - 1 - j; // itkImage grid convention is different from GSLib's
                        GaborUtils::realType rValue = responseRealPart->GetPixel( index );
                        GaborUtils::realType iValue = responseImaginaryPart->GetPixel( index );
                        std::complex<GaborUtils::realType> cValue( rValue, iValue );
                        (*m_spectrogram)( i, j, s1 - s0 - step ) += std::abs( cValue );
                }

            if( singleAzimuth || azimuth > azimuthFinalIfNotSingle )
                break;
            azimuth += 5.0;
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
