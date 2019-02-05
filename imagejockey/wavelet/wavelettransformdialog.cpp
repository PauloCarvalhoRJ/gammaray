//----------Since we're not building with CMake, we need to init the VTK modules------------------
//--------------linking with the VTK libraries is often not enough--------------------------------
#include <QInputDialog>
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2) // VTK was built with vtkRenderingOpenGL2
VTK_MODULE_INIT(vtkInteractionStyle)
VTK_MODULE_INIT(vtkRenderingFreeType)
//------------------------------------------------------------------------------------------------


#include "wavelettransformdialog.h"
#include "ui_wavelettransformdialog.h"

#include "spectral/spectral.h"
#include "imagejockey/svd/svdfactor.h"
#include "imagejockey/widgets/ijgridviewerwidget.h"
#include "imagejockey/imagejockeyutils.h"

#include <QMessageBox>
#include <QVTKOpenGLWidget.h>
#include <vtkRenderer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkColorTransferFunction.h>
#include <vtkLookupTable.h>
#include <vtkDataSetMapper.h>
#include <vtkImageData.h>
#include <vtkActor2D.h>
#include <vtkCubeAxesActor2D.h>
#include <vtkScalarBarActor.h>
#include <vtkUnstructuredGrid.h>
#include <vtkHexahedron.h>
#include <vtkProperty.h>
#include <vtkIntArray.h>
#include <vtkFloatArray.h>
#include <vtkCellData.h>
#include <vtkThreshold.h>
#include <QLineSeries>
#include <QValueAxis>
#include <QChart>
#include <QChartView>
#include <QGraphicsLayout>

WaveletTransformDialog::WaveletTransformDialog(IJAbstractCartesianGrid *inputGrid, uint inputVariableIndex, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WaveletTransformDialog),
    m_inputGrid( inputGrid ),
    m_inputVariableIndex( inputVariableIndex )
{
    ui->setupUi(this);
    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle( "Wavelet Transform Dialog" );

    ui->splitter->setSizes(QList<int>() << 100 << 200);

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
    ui->layout3DViewer->addWidget( _vtkwidget );
    //////////////////////////////////////////////////////////////

    //this causes the wavelet type combobox to update
    onWaveletFamilySelected( ui->cmbWaveletFamily->currentText() );

    //////////////////////////////////////////////////////////////////

    double a[128];
    a[0] = 0.0;
    a[1] = 1.0;
    for( int i = 2; i < 128; ++i )
        a[i] = 0.0;
    WaveletUtils::backtrans( a, 7, WaveletFamily::DAUBECHIES, 4 );

    //create and fill a data series object for the chart
    QtCharts::QLineSeries *series = new QtCharts::QLineSeries();
    for(uint i = 0; i < 128; ++i){
        series->append( i+1, a[i] );
    }

    //create a new chart object using the data series
    QtCharts::QChart *chart = new QtCharts::QChart();
    chart->legend()->hide();
    chart->layout()->setContentsMargins(0, 0, 0, 0);
    chart->addSeries(series);
    chart->createDefaultAxes();
    QtCharts::QValueAxis *axisX = new QtCharts::QValueAxis();
    chart->setAxisX( axisX, series );
    QtCharts::QChartView* chartView = new QtCharts::QChartView( chart );
    ui->frmWaveletDisplay->layout()->addWidget( chartView );
    //////////////////////////////////////////////////////////////////////

    updateDisplay();
}

WaveletTransformDialog::~WaveletTransformDialog()
{
    delete ui;
}

WaveletFamily WaveletTransformDialog::getSelectedWaveletFamily()
{
    QString waveletFamilyName = ui->cmbWaveletFamily->currentText();
    WaveletFamily waveletFamily = WaveletFamily::UNKNOWN;
    if( waveletFamilyName == "Daubechies" ){
        waveletFamily = WaveletFamily::DAUBECHIES;
    } else if( waveletFamilyName == "Haar" ){
        waveletFamily = WaveletFamily::HAAR;
    } else if( waveletFamilyName == "B-Spline" ){
        waveletFamily = WaveletFamily::B_SPLINE;
    } else {
        QMessageBox::critical( this, "Error", QString("WaveletTransformDialog::getSelectedWaveletFamily(): Unknown wavelet family: " + waveletFamilyName));
    }
    return waveletFamily;
}

void WaveletTransformDialog::onPerformTransform()
{
    int waveletType = ui->cmbWaveletType->itemData( ui->cmbWaveletType->currentIndex() ).toInt();
    bool interleaved = ( ui->cmbMethod->currentIndex() == 0 );
    m_DWTbuffer = WaveletUtils::transform( m_inputGrid,
                                           m_inputVariableIndex,
                                           getSelectedWaveletFamily(),
                                           waveletType,
                                           interleaved );
    debugGrid( m_DWTbuffer );

    // set the color scale form fields to suitable initial values
    double absOfMin = std::abs( m_DWTbuffer.min() );
    double absOfMax = std::abs( m_DWTbuffer.max() );
    ui->txtColorScaleMin->setText( "0.0" );
    ui->txtColorScaleMax->setText( QString::number( std::max( absOfMax, absOfMin ) ) );
    ui->txtThresholdMin->setText ( QString::number( m_DWTbuffer.min() ) );
    ui->txtThresholdMax->setText ( "0.0" );
    ui->txtThresholdMin2->setText( "0.0" );
    ui->txtThresholdMax2->setText( QString::number( m_DWTbuffer.max() ) );

    // determine the number of levels (assumes the DWT transform is square).
    int numberOfLevels = std::log2( m_DWTbuffer.M() );

    // reconfigure the level spin boxes accoring to the number of levels.
    ui->spinLevelMin->setMinimum( 0 );
    ui->spinLevelMin->setMaximum( numberOfLevels - 1 );
    ui->spinLevelMin->setValue( 0 );
    ui->spinLevelMax->setMinimum( 0 );
    ui->spinLevelMax->setMaximum( numberOfLevels - 1 );
    ui->spinLevelMax->setValue( numberOfLevels - 1 );

    updateDisplay();
}

void WaveletTransformDialog::onWaveletFamilySelected( QString waveletFamilyName )
{
    //see which wavelet types are supported for each family in
    // http://www.gnu.org/software/gsl/manual/html_node/DWT-Initialization.html#DWT-Initialization
    ui->cmbWaveletType->clear();
    if( waveletFamilyName == "Daubechies" ){
        for( int i = 4; i < 21; i += 2)
            ui->cmbWaveletType->addItem( QString::number( i ), QVariant( i ) );
        return;
    }
    if( waveletFamilyName == "Haar" ){
        ui->cmbWaveletType->addItem( "2", QVariant( 2 ) );
        return;
    }
    if( waveletFamilyName == "B-Spline" ){
        //parameter is computed as i*100 + j
        ui->cmbWaveletType->addItem( "i=1, j=3", QVariant( 103 ));
        ui->cmbWaveletType->addItem( "i=1, j=5", QVariant( 105 ));
        ui->cmbWaveletType->addItem( "i=2, j=2", QVariant( 202 ));
        ui->cmbWaveletType->addItem( "i=2, j=4", QVariant( 204 ));
        ui->cmbWaveletType->addItem( "i=2, j=6", QVariant( 206 ));
        ui->cmbWaveletType->addItem( "i=2, j=8", QVariant( 208 ));
        ui->cmbWaveletType->addItem( "i=3, j=1", QVariant( 301 ));
        ui->cmbWaveletType->addItem( "i=3, j=3", QVariant( 303 ));
        ui->cmbWaveletType->addItem( "i=3, j=5", QVariant( 305 ));
        ui->cmbWaveletType->addItem( "i=3, j=7", QVariant( 307 ));
        ui->cmbWaveletType->addItem( "i=3, j=9", QVariant( 309 ));
        return;
    }
    QMessageBox::critical( this, "Error", QString("WaveletTransformDialog::onWaveletFamilySelected(): Unknown wavelet family: " + waveletFamilyName));
}

void WaveletTransformDialog::onSaveDWTResultAsGrid()
{
    if( m_DWTbuffer.size() > 0 && ! ui->txtCoeffVariableName->text().trimmed().isEmpty() )
        emit saveDWTTransform( ui->txtCoeffVariableName->text(), m_DWTbuffer );
    else
        QMessageBox::critical( this, "Error", "No data to save or grid name not given.");
}

void WaveletTransformDialog::onReadDWTResultFromGrid()
{
    if( m_DWTbuffer.size() > 0 && ! ui->txtCoeffVariableName->text().trimmed().isEmpty() ){
        IJAbstractCartesianGrid* pointerToRequestedGrid;
        emit requestGrid( ui->txtCoeffVariableName->text(), pointerToRequestedGrid );
        if( ! pointerToRequestedGrid ){
            QMessageBox::critical( this, "Error", "Null data grid.");
            return;
        }
        //read the requested data
        int nI = pointerToRequestedGrid->getNI();
        int nJ = pointerToRequestedGrid->getNJ();
        for( int i = 0; i < nI; ++i )
            for( int j = 0; j < nJ; ++j )
                m_DWTbuffer( i, j, 0 ) = pointerToRequestedGrid->getData( 0, i, j, 0 );
    }else
        QMessageBox::critical( this, "Error", "No array to receive the data or grid name not given.");
}

void WaveletTransformDialog::onPreviewBacktransformedResult()
{
    int waveletType = ui->cmbWaveletType->itemData( ui->cmbWaveletType->currentIndex() ).toInt();
    bool interleaved = ( ui->cmbMethod->currentIndex() == 0 );
    spectral::array backtrans = WaveletUtils::backtrans( m_inputGrid,
                                                         m_DWTbuffer,
                                                         getSelectedWaveletFamily(),
                                                         waveletType,
                                                         interleaved );
    debugGrid( backtrans );
}

void WaveletTransformDialog::updateDisplay()
{
    //Clear current display
    clearDisplay();

    //Get user settings.
    double colorScaleMin = ui->txtColorScaleMin->text().toDouble();
    double colorScaleMax = ui->txtColorScaleMax->text().toDouble();
    double thresholdMinValue1 = ui->txtThresholdMin->text().toDouble();
    double thresholdMaxValue1 = ui->txtThresholdMax->text().toDouble();
    double thresholdMinValue2 = ui->txtThresholdMin2->text().toDouble();
    double thresholdMaxValue2 = ui->txtThresholdMax2->text().toDouble();
    int minLevel = ui->spinLevelMin->value();
    int maxLevel = ui->spinLevelMax->value();

    /////-----------------code to render the input grid (aid in interpretation) -------------------
    vtkSmartPointer<vtkActor> gridActor = vtkSmartPointer<vtkActor>::New();
    {
        //load the input data as an array
        m_inputGrid->dataWillBeRequested();
        spectral::arrayPtr inputAsArray( m_inputGrid->createSpectralArray( m_inputVariableIndex ) );

        //convert the array into an ITK image object
        GaborUtils::ImageTypePtr inputAsITK = GaborUtils::convertSpectralArrayToITKImage( *inputAsArray );

        //mirror pad the image to the needed dimension (power of 2)
        int nPowerOf2;
        GaborUtils::ImageTypePtr inputMirrorPadded = WaveletUtils::squareAndMirrorPad( inputAsITK, nPowerOf2 );

        //convert the padded square image to array object.
        spectral::array inputMirrorPaddedAsArray =
                GaborUtils::convertITKImageToSpectralArray( *inputMirrorPadded );

        //convert the padded square image array into VTK grid.
        vtkSmartPointer<vtkImageData> out = vtkSmartPointer<vtkImageData>::New();
        ImageJockeyUtils::makeVTKImageDataFromSpectralArray( out, inputMirrorPaddedAsArray );

        //get max and min of input values for the color scale
        double colorScaleMin = m_inputGrid->getMin( m_inputVariableIndex );
        double colorScaleMax = m_inputGrid->getMax( m_inputVariableIndex );

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

    /////--------------------code to render the scalogram cubes-----------------------
    vtkSmartPointer<vtkActor> scalogramActor = vtkSmartPointer<vtkActor>::New();
    vtkSmartPointer<vtkScalarBarActor> scalarBarActor = vtkSmartPointer<vtkScalarBarActor>::New();
    {
        //  GSL's wavelet transform outputs the data of the 3D scalogram reusing the original
        //  2D grid like this (example is 16 x 16 cells):
        //  |----------------|----------------|
        //  |                |                |
        //  |                |                |
        //  |                |                |   Convention:
        //  |       3a       |      3c        |      -The numbers are the levels.  For a
        //  |                |                |       16x16 input, we have four levels (0 through 3).
        //  |                |                |       A higher level means more detail (higher frequency).
        //  |                |                |      -The letters are directions. a = N-S; b = E-W
        //  |--------|-------|----------------|       c = both diagonals.  This is because a 2D DWT is actually
        //  |        |       |                |       two 1D DWT's column-wise and row-wise. Thus we have
        //  |   2a   |  2c   |                |       three scalograms.
        //  |        |       |                |      -The cells in lower levels should be represented in rendering
        //  |----|---|-------|      3b        |       by doubling the cell size.  If the cell size in the original
        //  | 1a | 1c|       |                |       grid is 10m x 10m, the cells in level 3 should be rendered with
        //  |--|-|---|  2b   |                |       20m x 20m cells; cells in level 2, with 40m x 40m and so on.
        //  |--|-| 1b|       |                |       Each level should be rendered at a different z-level, forming a cube.
        //  |--|-|---|-------|----------------|
        //    ^ ^
        //    | |--- These three are 0a, 0c and 0b
        //    |----- This single cell stores the smooth factor, a value representing the global trend
        //           It is not part of the scalogram, though it used in the transform.  Thus the total number
        //           of cells in the cubes is the number of cells of the input grid minus 1.
        //
        //  The three scalograms should be rendered in 3D like this:
        //
        //               /----------------/  /----------------/
        //              /                /| /                /|  level 3
        //             /  direction a   /  /   direction c  / |
        //            /                /  /                / /|  level 2
        //           /----------------/  /----------------/ / |
        //           |__|_|_|_|_|_|_|_|  |__|_|_|_|_|_|_|_|/ /|  level 1
        //           |    |   |   |   |/----------------/ | / |
        //           |____|___|___|___/                /|_|/ /|  level 0
        //           |        |      /  direction b   / | | / |
        //           |________|_____/                / /|_|/ /
        //           |             /----------------/ / | | /
        //           |_____________|__|_|_|_|_|_|_|_|/ /|_|/
        //                         |    |   |   |   | / |
        //                         |____|___|___|___|/ /|
        //                         |        |       | / |
        //                         |________|_______|/ /
        //                         |                | /
        //                         |________________|/
        //

        // determine the number of levels (assumes the DWT transform is square).
        int numberOfLevels = std::log2( m_DWTbuffer.M() );

        // determine the number of cells in the three cubes (assumes the DWT transform is square).
        int numberOfCells = m_DWTbuffer.M() * m_DWTbuffer.M() - 1;

        double gridCellWidth  = m_inputGrid->getCellSizeI();
        double gridCellLength = m_inputGrid->getCellSizeJ();
        double gridWidth      = gridCellWidth * m_DWTbuffer.M();
        double gridLength     = gridCellLength * m_DWTbuffer.N();
        double gridX0         = m_inputGrid->getOriginX();
        double gridY0         = m_inputGrid->getOriginY();
        double gridZ0         = m_inputGrid->getOriginZ();

        // Create a VTK container with the points (mesh vertexes)
        vtkSmartPointer< vtkPoints > hexahedraPoints = vtkSmartPointer< vtkPoints >::New();
        hexahedraPoints->SetNumberOfPoints( numberOfCells * 8 );
        // for each level (0, 1, 2, 3, ...) of the scalograms
        int vertexIndex = 0;
        for( int iLevel = 0; iLevel < numberOfLevels; ++iLevel ){
            int numberOfRowsInLevel = ( 1 << iLevel );
            int numberOfColsInLevel = numberOfRowsInLevel;
            double cellWidth = gridWidth / numberOfColsInLevel;
            double cellLength = gridLength / numberOfRowsInLevel;
            double cellHeight = gridWidth / numberOfLevels; //2D: make cell hight equal one of the areal grid sizes divided by the number of levels so the scalograms look like cubes
            //for each scalogram (a, b and c) = (N-S, E-W, diagonals)
            for( char cScalogram = 'a'; cScalogram <= 'c'; ++cScalogram ){
                for( int jCell = 0; jCell < numberOfColsInLevel; ++jCell ){
                    for( int iCell = 0; iCell < numberOfRowsInLevel; ++iCell ){
                        double xOffset = 0;
                        double yOffset = 0;
                        if( cScalogram == 'b' || cScalogram == 'c' )
                            xOffset = gridWidth  + gridCellWidth * 10.0; //this control the E-W spacing between the scalograms and the original grid in the scene
                        if( cScalogram == 'a' || cScalogram == 'c' )
                            yOffset = gridLength + gridCellLength * 10.0; //this control the N-S spacing between the scalograms and the original grid in the scene
                        double cellX0 = gridX0 + xOffset + cellWidth*jCell;
                        double cellX1 = cellX0 + cellWidth;
                        double cellY0 = gridY0 + yOffset + cellLength*iCell;
                        double cellY1 = cellY0 + cellLength;
                        double cellZ0 = gridZ0 + cellHeight*iLevel;
                        double cellZ1 = cellZ0 + cellHeight;
                        hexahedraPoints->InsertPoint( ++vertexIndex, cellX0, cellY0, cellZ0 );
                        hexahedraPoints->InsertPoint( ++vertexIndex, cellX1, cellY0, cellZ0 );
                        hexahedraPoints->InsertPoint( ++vertexIndex, cellX1, cellY1, cellZ0 );
                        hexahedraPoints->InsertPoint( ++vertexIndex, cellX0, cellY1, cellZ0 );
                        hexahedraPoints->InsertPoint( ++vertexIndex, cellX0, cellY0, cellZ1 );
                        hexahedraPoints->InsertPoint( ++vertexIndex, cellX1, cellY0, cellZ1 );
                        hexahedraPoints->InsertPoint( ++vertexIndex, cellX1, cellY1, cellZ1 );
                        hexahedraPoints->InsertPoint( ++vertexIndex, cellX0, cellY1, cellZ1 );
                    }
                }
            }
        }

        // Create a VTK unstructured grid object (allows cells of arbitrary shapes )
        vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
        unstructuredGrid->Allocate( numberOfCells );
        vertexIndex = 0;
        for( uint i = 0; i < numberOfCells; ++i ) {
            vtkSmartPointer< vtkHexahedron > hexahedron = vtkSmartPointer< vtkHexahedron >::New();
            hexahedron->GetPointIds()->SetId(0, ++vertexIndex);
            hexahedron->GetPointIds()->SetId(1, ++vertexIndex);
            hexahedron->GetPointIds()->SetId(2, ++vertexIndex);
            hexahedron->GetPointIds()->SetId(3, ++vertexIndex);
            hexahedron->GetPointIds()->SetId(4, ++vertexIndex);
            hexahedron->GetPointIds()->SetId(5, ++vertexIndex);
            hexahedron->GetPointIds()->SetId(6, ++vertexIndex);
            hexahedron->GetPointIds()->SetId(7, ++vertexIndex);
            unstructuredGrid->InsertNextCell(hexahedron->GetCellType(), hexahedron->GetPointIds());
        }
        unstructuredGrid->SetPoints(hexahedraPoints);

        //create a VTK array to store the sample values
        vtkSmartPointer<vtkFloatArray> values = vtkSmartPointer<vtkFloatArray>::New();
        values->SetName("values");

        //create a visibility array. Cells with visibility >= 1 will be
        //visible, and < 1 will be invisible.
        vtkSmartPointer<vtkIntArray> visibility = vtkSmartPointer<vtkIntArray>::New();

        //read sample values (order must be done so we can achive the rendering as
        //explained further above).
        values->Allocate( numberOfCells );
        visibility->Allocate( numberOfCells );
        visibility->SetNumberOfComponents(1);
        visibility->SetName("Visibility");
        // for each level (0, 1, 2, 3, ...) of the scalograms
        for( int iLevel = 0; iLevel < numberOfLevels; ++iLevel ){
            //for each scalogram (a, b and c) = (N-S, E-W, diagonals)
            for( char cScalogram = 'a'; cScalogram <= 'c'; ++cScalogram ){
                //compute the input grid indexes ranges to read the data
                //from the correct sub-grid corresponding to current level and current
                //scalogram
                int levelDim = 1 << iLevel;
                int iStart = 0;
                int jStart = 0;
                if( cScalogram == 'b' || cScalogram == 'c' )
                    iStart = levelDim;
                if( cScalogram == 'a' || cScalogram == 'c' )
                    jStart = levelDim;
                int iEnd = iStart + levelDim;
                int jEnd = jStart + levelDim;
                //read the values from the sub-grid
                for( int jCell = jStart; jCell < jEnd; ++jCell ){
                    for( int iCell = iStart; iCell < iEnd; ++iCell ){
                        double value = m_DWTbuffer( jCell, iCell, 0 ); // i index is N-S-wise (see the loop to build geometry further above)
                        //assign the coefficient to the data array
                        values->InsertNextValue( std::abs( value ) );
                        //set the visibility flag accoring to the filter settings
                        if ( ( ( value >= thresholdMinValue1 && value <= thresholdMaxValue1 ) ||
                               ( value >= thresholdMinValue2 && value <= thresholdMaxValue2 ) ) &&
                               iLevel >= minLevel && iLevel <= maxLevel )
                            visibility->InsertNextValue( 1 );
                        else
                            visibility->InsertNextValue( 0 );
                    }
                }
            }
        }

        //assign the grid values to the grid cells
        unstructuredGrid->GetCellData()->SetScalars( values );

        //assign the visibility flags to the grid cells
        unstructuredGrid->GetCellData()->AddArray( visibility );

        // configure a thresholding object to make cells below or above cut-offs invisible
        vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
        {
            threshold->SetInputData( unstructuredGrid );
            threshold->ThresholdByUpper(1); // Criterion is cells whose flags are greater or equal to threshold.
            threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Visibility");
            threshold->Update();
        }

        //Create a color table
        vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
        {
            size_t tableSize = 64; //number of shades
            //create a color interpolator object (grayscale)
            vtkSmartPointer<vtkColorTransferFunction> ctf = vtkSmartPointer<vtkColorTransferFunction>::New();
            ctf->SetColorSpaceToRGB();
            ctf->AddRGBPoint(0.00, 0.000, 0.000, 0.000);
            ctf->AddRGBPoint(0.33, 1.000, 1.000, 0.000);
            ctf->AddRGBPoint(0.66, 1.000, 0.000, 0.000);
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

        //Confgure the coefficient values scale bar
        scalarBarActor->SetLookupTable( lut );
        scalarBarActor->SetTitle("abs(coefficients)");
        scalarBarActor->SetNumberOfLabels( 5 );

        // Create mapper (visualization parameters)
        vtkSmartPointer<vtkDataSetMapper> mapper =
                vtkSmartPointer<vtkDataSetMapper>::New();
        mapper->SetInputConnection( threshold->GetOutputPort() );
        mapper->SetLookupTable(lut);
        mapper->SetScalarRange(colorScaleMin, colorScaleMax);
        mapper->Update();

        // Finally, pass everything to the actor and return it.
        scalogramActor->SetMapper(mapper);
//        scalogramActor->GetProperty()->EdgeVisibilityOn();
//        scalogramActor->GetProperty()->BackfaceCullingOn();
//        scalogramActor->GetProperty()->FrontfaceCullingOn();
    }


    //Update the graphics system.
    _renderer->AddActor( scalogramActor );
    _currentActors.push_back( scalogramActor );
    _renderer->AddActor2D( scalarBarActor );
    _scaleBarActor = scalarBarActor;
    _renderer->AddActor( gridActor );
    _currentActors.push_back( gridActor );
    _renderer->ResetCamera();
    _vtkwidget->GetRenderWindow()->Render();
}

void WaveletTransformDialog::onUpdateWaveletDisplays()
{

}

void WaveletTransformDialog::debugGrid(const spectral::array &grid)
{
    spectral::array result ( grid );
    SVDFactor* gridSVD = new SVDFactor( std::move(result), 1, 0.42,
                                     0.0,
                                     0.0,
                                     0.0,
                                     1.0,
                                     1.0,
                                     1.0, 0.0 );
    IJGridViewerWidget* ijgv = new IJGridViewerWidget( true, false, true );
    ijgv->setFactor( gridSVD );
    ijgv->show();
}

void WaveletTransformDialog::clearDisplay()
{
    std::vector< vtkSmartPointer<vtkActor> >::iterator it = _currentActors.begin();
    for( ; it != _currentActors.end(); ){ // erase() already increments the iterator.
        _renderer->RemoveActor( *it );
        it = _currentActors.erase( it );
    }
    _renderer->RemoveActor( _scaleBarActor );
    _renderer->RemoveViewProp( _axes.GetPointer() );
}
