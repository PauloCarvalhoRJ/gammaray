#include "emdanalysisdialog.h"
#include "ui_emdanalysisdialog.h"
#include "spectral/spectral.h"
#include "imagejockey/widgets/ijgridviewerwidget.h"
#include "imagejockey/svd/svdfactor.h"
#include "imagejockey/widgets/ijquick3dviewer.h"
#include <vtkSmartPointer.h>
#include <vtkPoints.h>
#include <vtkDoubleArray.h>
#include <vtkShepardMethod.h>
#include <vtkImageData.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <QMessageBox>

EMDAnalysisDialog::EMDAnalysisDialog(IJAbstractCartesianGrid *inputGrid, uint inputVariableIndex, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EMDAnalysisDialog),
    m_inputGrid( inputGrid ),
    m_inputVariableIndex( inputVariableIndex )
{
    ui->setupUi(this);

    setWindowTitle( "Empirical Mode Decomposition" );

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);
}

EMDAnalysisDialog::~EMDAnalysisDialog()
{
    delete ui;
}

void EMDAnalysisDialog::onPerformEMD()
{
    //fetch data from file
    m_inputGrid->dataWillBeRequested();

    //get grid dimensions
    int nI = m_inputGrid->getNI();
    int nJ = m_inputGrid->getNJ();
    int nK = m_inputGrid->getNK();
    double dx = m_inputGrid->getCellSizeI();
    double dy = m_inputGrid->getCellSizeJ();
    double dz = m_inputGrid->getCellSizeK();

    //get the null data value (NaN for a spectral::array single-variable grid)
    double NDV = std::numeric_limits<double>::quiet_NaN();

    //create the local maxima envelope data set, initialized with no-data-values.
    spectral::array localMaximaEnvelope( nI, nJ, nK, NDV );

    //create the local minima envelope data set, initialized with no-data-values.
    spectral::array localMinimaEnvelope( nI, nJ, nK, NDV );

    //define the size of half-window (1 means a 3x3x3 window with the target cell in the center)
    int halfWindowSize = 1;

    //initialize the current Empirical Mode Function with the grid's original data
    spectral::array* currentEMF = m_inputGrid->createSpectralArray( m_inputVariableIndex );

    //EMD iterations
    {
        //count the local extrema count
        uint localMaximaCount = 0;
        uint localMinimaCount = 0;

        //-------------- The loop to find the local extrema for the envelopes -----------------
        //for each cell...
        for( int k = 0; k < nK; ++k )
            for( int j = 0; j < nJ; ++j )
                for( int i = 0; i < nI; ++i ){
                    bool is_a_local_minimum = true;
                    bool is_a_local_maximum = true;
                    //...get its value
                    double cellValue = (*currentEMF)( i, j, k );
                    //...evaluate the neighboring values
                    for( int offsetK = -halfWindowSize; offsetK <= halfWindowSize; ++offsetK )
                        for( int offsetJ = -halfWindowSize; offsetJ <= halfWindowSize; ++offsetJ )
                            for( int offsetI = -halfWindowSize; offsetI <= halfWindowSize; ++offsetI ){
                                int neighI = i + offsetI;
                                int neighJ = j + offsetJ;
                                int neighK = k + offsetK;
                                //if's to handle border cases
                                if( neighI >= 0 && neighI < nI &&
                                    neighJ >= 0 && neighJ < nJ &&
                                    neighK >= 0 && neighK < nK &&
                                    ! (neighI == i && neighJ == j && neighK == k) /*do not compare with itself*/ ){
                                    //get neighboring value
                                    double neighValue = (*currentEMF)( neighI, neighJ, neighK );
                                    //if it is a valid value
                                    if( std::isfinite( neighValue )){
                                        //if the cell value is less than a neighbor's...
                                        if( cellValue < neighValue )
                                            //...it can't be a local maximum
                                            is_a_local_maximum = false;
                                        //if the cell value is greater than a neighbor's...
                                        if( cellValue > neighValue )
                                            //...it can't be a local minimum
                                            is_a_local_minimum = false;
                                    }
                                }
                            } // --- evaluate the neighboring cells
                    //if the cell is a local maximum...
                    if( is_a_local_maximum ){
                        //... assign the value to the grid of the local maxima envelope
                        localMaximaEnvelope( i, j, k ) = cellValue;
                        ++localMaximaCount;
                    }
                    //if the cell is a local minimum...
                    if( is_a_local_minimum ){
                        //... assign the value to the grid of the local minima envelope
                        localMinimaEnvelope( i, j, k ) = cellValue;
                        ++localMinimaCount;
                    }
                } // --- for each cell

        if( localMaximaCount < ui->spinMinNbOfExtrema->value() ){
            QMessageBox::information( this, "Info", "EMD terminated by reaching minimum number of local maxima.");
            return;
        }

        if( localMinimaCount < ui->spinMinNbOfExtrema->value() ){
            QMessageBox::information( this, "Info", "EMD terminated by reaching minimum number of local minima.");
            return;
        }

        //----------------------------------------------------------------------------------------

    //    IJGridViewerWidget* ijgw = new IJGridViewerWidget( true, false, true, this );
    //    SVDFactor* grid = new SVDFactor( std::move(localMaximaEnvelope), 1, 1.0, 0.0, 0.0, 0.0,
    //                                     m_inputGrid->getCellSizeI(),
    //                                     m_inputGrid->getCellSizeJ(),
    //                                     m_inputGrid->getCellSizeK(),
    //                                     0.42
    //                                     );
    //    ijgw->setFactor( grid );
    //    ijgw->show();
    //    return;

        //--------------------------interpolate the local extrema--------------------------------

        //the VTK collections of points in space of the local extrema
        vtkSmartPointer<vtkPoints> pointsMaxima = vtkSmartPointer<vtkPoints>::New();
        vtkSmartPointer<vtkPoints> pointsMinima = vtkSmartPointer<vtkPoints>::New();
        pointsMaxima->SetNumberOfPoints( localMaximaCount );
        pointsMinima->SetNumberOfPoints( localMinimaCount );

        //the VTK collections of values of local extrema
        vtkSmartPointer<vtkDoubleArray> valuesMaxima = vtkSmartPointer<vtkDoubleArray>::New();
        vtkSmartPointer<vtkDoubleArray> valuesMinima = vtkSmartPointer<vtkDoubleArray>::New();
        valuesMaxima->SetNumberOfComponents(1);
        valuesMaxima->SetNumberOfValues( localMaximaCount );
        valuesMaxima->SetName("ValuesMaxima");
        valuesMinima->SetNumberOfComponents(1);
        valuesMaxima->SetNumberOfValues( localMinimaCount );
        valuesMinima->SetName("ValuesMinima");

        //the VTK collections of ids to identify vertexes
        vtkSmartPointer<vtkIdList> vIDsMaxima = vtkSmartPointer<vtkIdList>::New();
        vtkSmartPointer<vtkIdList> vIDsMinima = vtkSmartPointer<vtkIdList>::New();
        vtkSmartPointer<vtkCellArray> vertexesMaxima = vtkSmartPointer<vtkCellArray>::New();
        vtkSmartPointer<vtkCellArray> vertexesMinima = vtkSmartPointer<vtkCellArray>::New();

        //populate the VTK collections above
        double x, y, z;
        for( int k = 0; k < nK; ++k )
            for( int j = 0; j < nJ; ++j )
                for( int i = 0; i < nI; ++i ){
                    double maximum = localMaximaEnvelope( i, j, k );
                    if( std::isfinite( maximum ) ){
                        m_inputGrid->getCellLocation( i, j, k, x, y, z );
                        vIDsMaxima->InsertNextId( pointsMaxima->InsertNextPoint( x, y, z ) );
                        valuesMaxima->InsertNextValue( maximum );
                    }
                    double minimum = localMinimaEnvelope( i, j, k );
                    if( std::isfinite( minimum ) ){
                        m_inputGrid->getCellLocation( i, j, k, x, y, z );
                        vIDsMinima->InsertNextId( pointsMinima->InsertNextPoint( x, y, z ) );
                        valuesMinima->InsertNextValue( minimum );
                    }
                }
        vertexesMaxima->InsertNextCell( vIDsMaxima );
        vertexesMinima->InsertNextCell( vIDsMinima );

        //mount VTK polygonal objects for interpolation with the extrema locations and values
        vtkSmartPointer<vtkPolyData> polydataMaxima = vtkSmartPointer<vtkPolyData>::New();
        polydataMaxima->SetPoints( pointsMaxima );
        polydataMaxima->SetVerts( vertexesMaxima );
        polydataMaxima->GetPointData()->SetScalars( valuesMaxima );
        vtkSmartPointer<vtkPolyData> polydataMinima = vtkSmartPointer<vtkPolyData>::New();
        polydataMinima->SetPoints( pointsMinima );
        polydataMinima->SetVerts( vertexesMinima );
        polydataMinima->GetPointData()->SetScalars( valuesMinima );

        //configure the volume for Shepard's Method interpolation algorithm
        //vtkGaussianSplatter can be an alternative if results are not good
        vtkSmartPointer<vtkShepardMethod> shepard = vtkSmartPointer<vtkShepardMethod>::New();
        shepard->SetInputData( polydataMinima );
        shepard->SetMaximumDistance(1); //1.0 means it uses all points (slower, but without search neighborhood artifacts)
        double xmin = m_inputGrid->getOriginX() - dx / 2.0;
        double ymin = m_inputGrid->getOriginY() - dy / 2.0;
        double zmin = m_inputGrid->getOriginZ() - dz / 2.0;
        double xmax = xmin + dx * nI;
        double ymax = ymin + dy * nJ;
        double zmax = zmin + dz * nK;
        shepard->SetModelBounds( xmin, xmax, ymin, ymax, zmin, zmax);
        shepard->SetSampleDimensions( nI, nJ, nK+1 );
        shepard->Update();

        vtkSmartPointer<vtkImageData> output = shepard->GetOutput();

        IJQuick3DViewer* ijq3dv = new IJQuick3DViewer;
        ijq3dv->setWindowTitle( "Maxima Envelope" );
        ijq3dv->show();
        ijq3dv->display( output,
                         m_inputGrid->getMin( m_inputVariableIndex ),
                         m_inputGrid->getMax( m_inputVariableIndex ) );

        //---------------------------------------------------------------------------------------

    } //----EMD iterations
}
