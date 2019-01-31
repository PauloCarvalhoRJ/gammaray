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

    /////-----------------code to render the input grid (aid in interpretation)-------------------
    vtkSmartPointer<vtkActor> gridActor = vtkSmartPointer<vtkActor>::New();
    {
        m_inputGrid->dataWillBeRequested();

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
        vtkSmartPointer< vtkPoints > hexaPoints = vtkSmartPointer< vtkPoints >::New();
        hexaPoints->SetNumberOfPoints( numberOfCells * 8 );
        // for each level (0, 1, 2, 3, ...) of the scalograms
        for( int iLevel = 0; iLevel < numberOfLevels; ++iLevel ){
            int numberOfRowsInLevel = ( 1 << iLevel );
            int numberOfColsInLevel = numberOfRowsInLevel;
            double cellWidth = gridWidth / numberOfColsInLevel;
            double cellLength = gridLength / numberOfRowsInLevel;
            double cellHeight = gridCellWidth; //2D: make cell hight equal one of the areal sizes
            //for each scalogram (a, b and c) = (N-S, E-W, diagonals)
            for( char cScalogram = 'a'; cScalogram <= 'c'; ++cScalogram ){
                for( int jCell = 0; jCell < numberOfColsInLevel; ++jCell ){
                    for( int iCell = 0; iCell < numberOfRowsInLevel; ++iCell ){
                        double xOffset = 0;
                        double yOffset = 0;
                        if( cScalogram == 'b' || cScalogram == 'c' )
                            xOffset = gridWidth  + gridCellWidth;
                        if( cScalogram == 'a' || cScalogram == 'c' )
                            yOffset = gridLength + gridCellLength;
                        double cellX0 = gridX0 + xOffset + cellWidth*jCell;
                        double cellX1 = cellX0 + cellWidth;
                        double cellY0 = gridY0 + yOffset + cellLenght*iCell;
                        double cellY1 = cellY0 + cellLength;
                        double cellZ0 = gridZ0 + cellHeight*iLevel;
                        double cellZ1 = cellZ0 + cellHeight;
                        TODO_STORE_THE_EIGHT_VERTEXES_OF_THE_CELL;
                    }
                }
            }

//            double x, y, z;
//            geoGrid->getMeshVertexLocation( i, x, y, z );
//            hexaPoints->InsertPoint(i, x, y, z);
        }

        // Create a VTK unstructured grid object (allows faults, erosions, and other geologic discordances )
        vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
        uint nCells = geoGrid->getMeshNumberOfCells();
        unstructuredGrid->Allocate( nCells );
        for( uint i = 0; i < nCells; ++i ) {
            uint vIds[8];
            geoGrid->getMeshCellDefinition( i, vIds );
            vtkSmartPointer< vtkHexahedron > hexa = vtkSmartPointer< vtkHexahedron >::New();
            hexa->GetPointIds()->SetId(0, vIds[0]);
            hexa->GetPointIds()->SetId(1, vIds[1]);
            hexa->GetPointIds()->SetId(2, vIds[2]);
            hexa->GetPointIds()->SetId(3, vIds[3]);
            hexa->GetPointIds()->SetId(4, vIds[4]);
            hexa->GetPointIds()->SetId(5, vIds[5]);
            hexa->GetPointIds()->SetId(6, vIds[6]);
            hexa->GetPointIds()->SetId(7, vIds[7]);
            unstructuredGrid->InsertNextCell(hexa->GetCellType(), hexa->GetPointIds());
        }
        unstructuredGrid->SetPoints(hexaPoints);

        // Create mapper (visualization parameters)
        vtkSmartPointer<vtkDataSetMapper> mapper =
                vtkSmartPointer<vtkDataSetMapper>::New();
        mapper->SetInputData( unstructuredGrid );
        //mapper->SetInputConnection( threshold->GetOutputPort() );
        //mapper->SetLookupTable(lut);
        //mapper->SetScalarRange(min, max);
        mapper->Update();

        // Finally, pass everything to the actor and return it.
        vtkSmartPointer<vtkActor> scalogramActor =
                vtkSmartPointer<vtkActor>::New();
        scalogramActor->SetMapper(mapper);
        scalogramActor->GetProperty()->EdgeVisibilityOn();
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
