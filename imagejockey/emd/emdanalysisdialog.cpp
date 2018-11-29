#include "emdanalysisdialog.h"
#include "ui_emdanalysisdialog.h"
#include "spectral/spectral.h"
#include "imagejockey/widgets/ijgridviewerwidget.h"
#include "imagejockey/svd/svdfactor.h"
#include "imagejockey/widgets/ijquick3dviewer.h"
#include "imagejockey/imagejockeyutils.h"
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
    int previousLocalMaximaCount = 0;
    int previousLocalMinimaCount = 0;

    // the number of the empirical mode function.
    // EMF #0 is the original image itself
    int EMFnumber = 0;

    //EMD iterations
    for( int iteration = 0; iteration < nSteps; ++iteration ){
        //count the local extrema count
        int localMaximaCount = 0;
        int localMinimaCount = 0;

        //create the local extrema envelopes data sets, initialized with the local extrema.
        spectral::array localMaximaEnvelope;
        spectral::array localMinimaEnvelope;

        //initialize the extrema envelopes with the extrema points
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

        //perform some checks before proceeding to interpolation of the extrema points
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
        spectral::array interpolatedMaximaEnvelope;
        spectral::array interpolatedMinimaEnvelope;
        if( ui->cmbInterpolationMethod->currentText() == "Shepard" ){
            interpolatedMaximaEnvelope = ImageJockeyUtils::interpolateNullValuesShepard( localMaximaEnvelope,
                                                            *m_inputGrid,
                                                            ui->dblSpinPowerParameter->value(),
                                                            ui->dblSpinMaxDistance->value(),
                                                            NDV );

            interpolatedMinimaEnvelope = ImageJockeyUtils::interpolateNullValuesShepard( localMinimaEnvelope,
                                                            *m_inputGrid,
                                                            ui->dblSpinPowerParameter->value(),
                                                            ui->dblSpinMaxDistance->value(),
                                                            NDV );
        } else {
            int status;
            interpolatedMaximaEnvelope = ImageJockeyUtils::interpolateNullValuesThinPlateSpline( localMaximaEnvelope,
                                                            *m_inputGrid,
                                                            ui->dblSpinLambda->value(),
                                                            status );
            if( status ){
                QMessageBox::critical( this, "Error",
                                       "EMD terminated because interpolation with Thin Plate Spline terminated prematurely. termination code = " +
                                       QString::number( status ));
                return;
            }
            interpolatedMinimaEnvelope = ImageJockeyUtils::interpolateNullValuesThinPlateSpline( localMinimaEnvelope,
                                                            *m_inputGrid,
                                                            ui->dblSpinLambda->value(),
                                                            status );
            if( status ){
                QMessageBox::critical( this, "Error",
                                       "EMD terminated because interpolation with Thin Plate Spline terminated prematurely. termination code = " +
                                       QString::number( status ));
                return;
            }
        }
        //Debug the interpolated envelopes
//        IJGridViewerWidget* ijgw = new IJGridViewerWidget( true, false, true, nullptr );
//        SVDFactor* grid2 = new SVDFactor( std::move(interpolatedMaximaEnvelope), 1, 1.0, 0.0, 0.0, 0.0, //DO NOT USE the array object beyond this point!
//                                         m_inputGrid->getCellSizeI(),
//                                         m_inputGrid->getCellSizeJ(),
//                                         m_inputGrid->getCellSizeK(),
//                                         0.42
//                                         );
//        ijgw->setFactor( grid2 );
//        ijgw->show();
//        IJGridViewerWidget* ijgw6 = new IJGridViewerWidget( true, false, true, nullptr );
//        SVDFactor* grid3 = new SVDFactor( std::move(interpolatedMinimaEnvelope), 1, 1.0, 0.0, 0.0, 0.0, //DO NOT USE the array object beyond this point!
//                                         m_inputGrid->getCellSizeI(),
//                                         m_inputGrid->getCellSizeJ(),
//                                         m_inputGrid->getCellSizeK(),
//                                         0.42
//                                         );
//        ijgw6->setFactor( grid3 );
//        ijgw6->show();
//        return;
        //---------------------------------------------------------------------------------------

        //----------------------------- Compute the next Empirical Mode Function-----------------
        for( int k = 0; k < nK; ++k )
            for( int j = 0; j < nJ; ++j )
                for( int i = 0; i < nI; ++i ){
                    (*currentEMF)( i, j, k ) = (*currentEMF)( i, j, k ) - ( interpolatedMaximaEnvelope(i,j,k) + interpolatedMinimaEnvelope(i,j,k) ) / 2.0 ;
                }
        ++EMFnumber;
        //---------------------------------------------------------------------------------------

        //Debug the next empirical mode function
        IJGridViewerWidget* ijgw2 = new IJGridViewerWidget( true, false, true );
        spectral::array currentEMFCopy( *currentEMF );
        SVDFactor* grid = new SVDFactor( std::move(currentEMFCopy), 1, 1.0, 0.0, 0.0, 0.0,
                                         m_inputGrid->getCellSizeI(),
                                         m_inputGrid->getCellSizeJ(),
                                         m_inputGrid->getCellSizeK(),
                                         0.42
                                         );
        ijgw2->setFactor( grid );
        ijgw2->setWindowTitle( "EMF #" + QString::number( EMFnumber ) );
        ijgw2->show();

        // keep track of number of extrema count to detect divergence
        previousLocalMaximaCount = localMaximaCount;
        previousLocalMinimaCount = localMinimaCount;
    } //----EMD iterations
}
