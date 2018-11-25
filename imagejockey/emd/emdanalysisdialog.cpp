#include "emdanalysisdialog.h"
#include "ui_emdanalysisdialog.h"
#include "spectral/spectral.h"
#include "imagejockey/widgets/ijgridviewerwidget.h"
#include "imagejockey/svd/svdfactor.h"
#include "imagejockey/widgets/ijquick3dviewer.h"
#include <vtkSmartPointer.h>
#include <vtkPoints.h>
#include <vtkFloatArray.h>
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

    //define the size of half-window (1 means a 3x3x3 window with the target cell in the center)
    int halfWindowSize = ui->spinHalfWindowSize->value();

    //initialize the current Empirical Mode Function with the grid's original data
    spectral::array* currentEMF = m_inputGrid->createSpectralArray( m_inputVariableIndex );

    //get the maximum number of steps
    int nSteps = ui->spinMaxNbOfSteps->value();

    // the local extrema counts in the previous interation to detect whether
    // the number of extrema increased (algorithm is diverging)
    uint previousLocalMaximaCount = 0;
    uint previousLocalMinimaCount = 0;

    //EMD iterations
    for( int iteration = 0; iteration < nSteps; ++iteration ){
        //count the local extrema count
        int localMaximaCount = 0;
        int localMinimaCount = 0;

        //create the local extrema envelopes data sets, initialized with the local extrema.
        spectral::array localMaximaEnvelope;
        spectral::array localMinimaEnvelope;

        if( ui->cmbExtremaType->currentText() == "points" ){
            localMaximaEnvelope = spectral::get_extrema_cells( *currentEMF,
                                                               spectral::ExtremumType::MAXIMUM,
                                                               halfWindowSize,
                                                               localMaximaCount );
            localMinimaEnvelope = spectral::get_extrema_cells( *currentEMF,
                                                               spectral::ExtremumType::MINIMUM,
                                                               halfWindowSize,
                                                               localMinimaCount );
        } else {
            localMaximaEnvelope = spectral::get_ridges_or_valleys( *currentEMF,
                                                                   spectral::ExtremumType::MAXIMUM,
                                                                   halfWindowSize,
                                                                   localMaximaCount );
            localMinimaEnvelope = spectral::get_ridges_or_valleys( *currentEMF,
                                                                   spectral::ExtremumType::MINIMUM,
                                                                   halfWindowSize,
                                                                   localMinimaCount );
        }

        if( localMaximaCount < ui->spinMinNbOfExtrema->value() ){
            QMessageBox::information( this, "Info", "EMD terminated by reaching minimum number of local maxima.");
            return;
        }

        if( localMinimaCount < ui->spinMinNbOfExtrema->value() ){
            QMessageBox::information( this, "Info", "EMD terminated by reaching minimum number of local minima.");
            return;
        }

        if( iteration > 0 && ( localMaximaCount > previousLocalMaximaCount ||
                               localMinimaCount > previousLocalMinimaCount ) ) {
            QMessageBox::information( this, "Info", "EMD terminated because it started diverging (number of local extrema increased).");
            return;
        }

        //----------------------------------------------------------------------------------------

        //Debug the extrema envelopes
//        IJGridViewerWidget* ijgw = new IJGridViewerWidget( true, false, true, nullptr );
//        SVDFactor* grid2 = new SVDFactor( std::move(localMinimaEnvelope), 1, 1.0, 0.0, 0.0, 0.0, //DO NOT USE localMaximaEnvelope beyond this point!
//                                         m_inputGrid->getCellSizeI(),
//                                         m_inputGrid->getCellSizeJ(),
//                                         m_inputGrid->getCellSizeK(),
//                                         0.42
//                                         );
//        ijgw->setFactor( grid2 );
//        ijgw->show();
//        return;

        //--------------------------interpolate the local extrema--------------------------------

        //the VTK collections of points in space of the local extrema
        vtkSmartPointer<vtkPoints> pointsMaxima = vtkSmartPointer<vtkPoints>::New();
        vtkSmartPointer<vtkPoints> pointsMinima = vtkSmartPointer<vtkPoints>::New();

        vtkSmartPointer<vtkCellArray> vertexesMaxima = vtkSmartPointer<vtkCellArray>::New();
        vtkSmartPointer<vtkCellArray> vertexesMinima = vtkSmartPointer<vtkCellArray>::New();

        //the VTK collections of values of local extrema
        vtkSmartPointer<vtkFloatArray> valuesMaxima = vtkSmartPointer<vtkFloatArray>::New();
        valuesMaxima->SetName("ValuesMaxima");
        vtkSmartPointer<vtkFloatArray> valuesMinima = vtkSmartPointer<vtkFloatArray>::New();
        valuesMinima->SetName("ValuesMinima");

        //the VTK collections of ids to identify vertexes
        vtkSmartPointer<vtkIdList> vIDsMaxima = vtkSmartPointer<vtkIdList>::New();
        vIDsMaxima->Allocate( localMaximaCount );
        vtkSmartPointer<vtkIdList> vIDsMinima = vtkSmartPointer<vtkIdList>::New();
        vIDsMaxima->Allocate( localMinimaCount );

        valuesMaxima->Allocate( localMaximaCount );
        valuesMinima->Allocate( localMinimaCount );

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
        //polydataMaxima->GetPointData()->SetActiveScalars("ValuesMaxima"); //setting this makes vtkShepardMethod fail... figures! But enable this to visualize.
        vtkSmartPointer<vtkPolyData> polydataMinima = vtkSmartPointer<vtkPolyData>::New();
        polydataMinima->SetPoints( pointsMinima );
        polydataMinima->SetVerts( vertexesMinima );
        polydataMinima->GetPointData()->SetScalars( valuesMinima );
        //polydataMaxima->GetPointData()->SetActiveScalars("ValuesMinima"); //setting this makes vtkShepardMethod fail... figures! But enable this to visualize.

        //Debug the extrema envelopes as point sets
//        IJQuick3DViewer* ijq3dv2 = new IJQuick3DViewer;
//        ijq3dv2->setWindowTitle( "Maxima Envelope" );
//        ijq3dv2->show();
//        ijq3dv2->display( polydataMaxima, 3.0f );
//        IJQuick3DViewer* ijq3dv3 = new IJQuick3DViewer;
//        ijq3dv3->setWindowTitle( "Minima Envelope" );
//        ijq3dv3->show();
//        ijq3dv3->display( polydataMinima, 3.0f );
//        return;

        //compute bounding box for Shepard' method
        double xmin = m_inputGrid->getOriginX() - dx / 2.0;
        double ymin = m_inputGrid->getOriginY() - dy / 2.0;
        double zmin = m_inputGrid->getOriginZ() - dz / 2.0;
        double xmax = xmin + dx * nI;
        double ymax = ymin + dy * nJ;
        double zmax = zmin + dz * nK;

        //configure the volume for Shepard's Method interpolation algorithm for the maxima envelope.
        //vtkGaussianSplatter can be an alternative if results are not good
        vtkSmartPointer<vtkShepardMethod> shepard = vtkSmartPointer<vtkShepardMethod>::New();
        shepard->SetInputData( polydataMaxima );
        shepard->SetMaximumDistance(1); //1.0 means it uses all points (slower, but without search neighborhood artifacts)
        shepard->SetModelBounds( xmin, xmax, ymin, ymax, zmin, zmax);
        shepard->SetPowerParameter( ui->dblSpinPowerParameter->value() );
        shepard->SetSampleDimensions( nI, nJ, nK+1 );
        shepard->SetNullValue( NDV );
        shepard->Update();
        vtkSmartPointer<vtkImageData> interpolatedMaximaEnvelope = shepard->GetOutput();

        //now re-configure Shepard's for the local minima envelope. (reusing the vtkShepardMethod instance didn't work)
        shepard = vtkSmartPointer<vtkShepardMethod>::New();
        shepard->SetInputData( polydataMinima );
        shepard->SetMaximumDistance(1); //1.0 means it uses all points (slower, but without search neighborhood artifacts)
        shepard->SetModelBounds( xmin, xmax, ymin, ymax, zmin, zmax);
        shepard->SetPowerParameter( ui->dblSpinPowerParameter->value() );
        shepard->SetSampleDimensions( nI, nJ, nK+1 );
        shepard->SetNullValue( NDV );
        shepard->Update();
        vtkSmartPointer<vtkImageData> interpolatedMinimaEnvelope = shepard->GetOutput();


        //Debug the interpolated extrema envelopes
//        IJQuick3DViewer* ijq3dv = new IJQuick3DViewer;
//        ijq3dv->setWindowTitle( "Maxima Envelope" );
//        ijq3dv->show();
//        ijq3dv->display( interpolatedMaximaEnvelope,
//                         m_inputGrid->getMin( static_cast<int>( m_inputVariableIndex ) ),
//                         m_inputGrid->getMax( static_cast<int>( m_inputVariableIndex ) ) );
//        IJQuick3DViewer* ijq3dv4 = new IJQuick3DViewer;
//        ijq3dv4->setWindowTitle( "Minima Envelope" );
//        ijq3dv4->show();
//        ijq3dv4->display( interpolatedMinimaEnvelope,
//                         m_inputGrid->getMin( static_cast<int>( m_inputVariableIndex ) ),
//                         m_inputGrid->getMax( static_cast<int>( m_inputVariableIndex ) ) );

        //---------------------------------------------------------------------------------------

        //----------------------------- Compute the next Empirical Mode Function-----------------

        for( int k = 0; k < nK; ++k )
            for( int j = 0; j < nJ; ++j )
                for( int i = 0; i < nI; ++i ){
                    //intput for VTK were vtkFloatArray's
                    float* cellMaxima = static_cast<float*>(interpolatedMaximaEnvelope->GetScalarPointer( i, j, k ));
                    float* cellMinima = static_cast<float*>(interpolatedMinimaEnvelope->GetScalarPointer( i, j, k ));
                    //assuming the first element in the returned array is the interpolated value
                    (*currentEMF)( i, j, k ) = (*currentEMF)( i, j, k ) - static_cast<double>(cellMaxima[0] + cellMinima[0]) / 2.0 ;
                }

        //---------------------------------------------------------------------------------------

        //Debug the extrema envelopes
        IJGridViewerWidget* ijgw2 = new IJGridViewerWidget( true, false, true );
        spectral::array currentEMFCopy( *currentEMF );
        SVDFactor* grid = new SVDFactor( std::move(currentEMFCopy), 1, 1.0, 0.0, 0.0, 0.0,
                                         m_inputGrid->getCellSizeI(),
                                         m_inputGrid->getCellSizeJ(),
                                         m_inputGrid->getCellSizeK(),
                                         0.42
                                         );
        ijgw2->setFactor( grid );
        ijgw2->show();

        // keep track of number of extrema count to detect divergence
        previousLocalMaximaCount = localMaximaCount;
        previousLocalMinimaCount = localMinimaCount;
    } //----EMD iterations
}
