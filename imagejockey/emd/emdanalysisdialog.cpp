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
#include <imagejockey/ijabstractvariable.h>

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

    //get the null data value (NaN for a spectral::array single-variable grid)
    double NDV = std::numeric_limits<double>::quiet_NaN();

    //define the size of half-window (1 means a 3x3x3 window with the target cell in the center)
    int halfWindowSize = ui->spinHalfWindowSize->value();

    //initialize the current Empirical Mode Function with the grid's original data
    spectral::array* currentSignal = m_inputGrid->createSpectralArray( m_inputVariableIndex );

    //get the maximum number of steps
    int nSteps = ui->spinMaxNbOfSteps->value();

    // the local extrema counts in the previous interation to detect whether
    // the number of extrema increased (algorithm is diverging)
    int previousLocalMaximaCount = 0;
    int previousLocalMinimaCount = 0;

    // the number of the empirical mode function.
    // IMF #0 is the original image itself
    int IMFnumber = 0;

    // this thresold discard extrema with low significant values.
    // this reduces the number of extrema while impact little the result.
    bool ok;
    double extremaThresholdAbs = ui->txtExtremaThresholdAbs->text().toDouble( &ok );
    if( ! ok ){
        QMessageBox::critical( this, "Info", "Invalid value entered for extrema threshold.");
        return;
    }

    // the value considered as "close enough to zero".
    double epsilon = ui->dblSpinEpsilon->value();

    spectral::array residueSignal(  (spectral::index)nI,
                                    (spectral::index)nJ,
                                    (spectral::index)nK,
                                    0.0);

    IJGridViewerWidget ijgw2( true, false, true );
    ijgw2.setWindowTitle( "mean envelope" );
    ijgw2.show();

    //EMD iterations
    for( int iteration = 0; iteration < nSteps; ++iteration ){
        //count the local extrema count
        int localMaximaCount = 0;
        int localMinimaCount = 0;

        //create the local extrema envelopes data sets, initialized with the local extrema.
        spectral::array localMaximaEnvelope;
        spectral::array localMinimaEnvelope;

        //initialize the extrema envelopes with the extrema points
        // Step (1) in Linderhed (2009)
        if( ui->cmbExtremaType->currentText() == "points" ){
            localMaximaEnvelope = spectral::get_extrema_cells( *currentSignal,
                                                               spectral::ExtremumType::MAXIMUM,
                                                               halfWindowSize,
                                                               extremaThresholdAbs,
                                                               localMaximaCount );
            localMinimaEnvelope = spectral::get_extrema_cells( *currentSignal,
                                                               spectral::ExtremumType::MINIMUM,
                                                               halfWindowSize,
                                                               extremaThresholdAbs,
                                                               localMinimaCount );
        } else {
            localMaximaEnvelope = spectral::get_ridges_or_valleys( *currentSignal,
                                                                   spectral::ExtremumType::MAXIMUM,
                                                                   halfWindowSize,
                                                                   extremaThresholdAbs,
                                                                   localMaximaCount );
            localMinimaEnvelope = spectral::get_ridges_or_valleys( *currentSignal,
                                                                   spectral::ExtremumType::MINIMUM,
                                                                   halfWindowSize,
                                                                   extremaThresholdAbs,
                                                                   localMinimaCount );
            //getting the ridges and valleys often result in thick lines, which means way more
            //samples than necessary to interpolate them.  These excess samples unnecessarily
            //increase the matrices in the interpolation steps.  Thus we need to get the center
            //lines of the valeys and ridges.
            localMaximaEnvelope = ImageJockeyUtils::skeletonize( localMaximaEnvelope );
            localMinimaEnvelope = ImageJockeyUtils::skeletonize( localMinimaEnvelope );
        }

        //perform some checks before proceeding to interpolation of the extrema points
        if( localMaximaCount < ui->spinMinNbOfExtrema->value() ||
            localMinimaCount < ui->spinMinNbOfExtrema->value() ){
            // save the last residue as a new variable to the grid data
            IJAbstractVariable* var = m_inputGrid->getVariableByIndex( m_inputVariableIndex );
            m_inputGrid->appendAsNewVariable( var->getVariableName() + "_RESIDUE",
                                              residueSignal );
            //save the last residue grid to file
            m_inputGrid->saveData();
            QMessageBox::information( this, "Info", "EMD terminated by reaching minimum number of local extrema.");
            return;
        }
        if( iteration > 0 && ( localMaximaCount > previousLocalMaximaCount ||
                               localMinimaCount > previousLocalMinimaCount ) ) {
            //QMessageBox::information( this, "Info", "EMD terminated because it started diverging (number of local extrema increased).");
            //return;
        }


        //Debug the extrema envelopes
//        IJGridViewerWidget* ijgw = new IJGridViewerWidget( true, false, true, nullptr );
//        SVDFactor* grid2 = new SVDFactor( std::move(localMaximaEnvelope), 1, 1.0, 0.0, 0.0, 0.0, //DO NOT USE localMaximaEnvelope beyond this point!
//                                         m_inputGrid->getCellSizeI(),
//                                         m_inputGrid->getCellSizeJ(),
//                                         m_inputGrid->getCellSizeK(),
//                                         0.42
//                                         );
//        ijgw->setFactor( grid2 );
//        ijgw->show();
//        return;


        //--------------------------interpolate the local extrema--------------------------------
        // Step (2) in Linderhed (2009)
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


        //-------compute the mean envelope--------------
        // Step (3) in Linderhed (2009)
        bool meanEnvelopIsNearZero = true;
        spectral::array meanEnvelope( (spectral::index)nI, (spectral::index)nJ, (spectral::index)nK );
        for( int k = 0; k < nK; ++k )
            for( int j = 0; j < nJ; ++j )
                for( int i = 0; i < nI; ++i ){
                    double meanValue = ( interpolatedMaximaEnvelope(i,j,k) + interpolatedMinimaEnvelope(i,j,k) ) / 2.0;
                    if( ! std::isfinite( meanValue ) ){
                        QMessageBox::critical( this, "Error",
                                               "Interpolation resulted in null values.  Maybe the search neighborhood is too small.");
                        return;
                    } else {
                        meanEnvelope( i, j, k ) = meanValue;
                        //If a mean value is too high, then current signal is not an IMF
                        if( std::abs(meanValue) > epsilon )
                            meanEnvelopIsNearZero = false;
                    }
                }

        //Debug the mean envelope
        spectral::array meanCopy( meanEnvelope );
        SVDFactor* grid = new SVDFactor( std::move(meanCopy), 1, 1.0, 0.0, 0.0, 0.0,
                                         m_inputGrid->getCellSizeI(),
                                         m_inputGrid->getCellSizeJ(),
                                         m_inputGrid->getCellSizeK(),
                                         0.42
                                         );
        ijgw2.setFactor( grid );
        QApplication::processEvents();


        //------------subtract the mean envelope from the current signal----------
        // Step (4) in Linderhed (2009)
        spectral::array candidateSignal = *currentSignal - meanEnvelope;

        //-----------check whether the mean envelope has value close to zero------
        //-----------if not, use the signal obtained in step (4), use it as
        //-----------input signal and start over.
        // Step (5) in Linderhed (2009)
        if( ! meanEnvelopIsNearZero ){
            (*currentSignal) = candidateSignal;
            --iteration; //no IMF found, step back
        } else { // candidate signal is an IMF
            //Debug the next empirical mode function
    //        IJGridViewerWidget* ijgw2 = new IJGridViewerWidget( true, false, true );
    //        spectral::array currentIMFCopy( *currentIMF );
    //        SVDFactor* grid = new SVDFactor( std::move(currentIMFCopy), 1, 1.0, 0.0, 0.0, 0.0,
    //                                         m_inputGrid->getCellSizeI(),
    //                                         m_inputGrid->getCellSizeJ(),
    //                                         m_inputGrid->getCellSizeK(),
    //                                         0.42
    //                                         );
    //        ijgw2->setFactor( grid );
    //        ijgw2->setWindowTitle( "IMF #" + QString::number( IMFnumber ) );
    //        ijgw2->show();
            ++IMFnumber;
            // save the candidate signal (an IMF) as a new variable to the grid data
            IJAbstractVariable* var = m_inputGrid->getVariableByIndex( m_inputVariableIndex );
            m_inputGrid->appendAsNewVariable( var->getVariableName() + "_IMF" + QString::number( IMFnumber ),
                                              candidateSignal );
            //save the IMF grid to file
            m_inputGrid->saveData();
            // compute the residue
            residueSignal = (*currentSignal) - candidateSignal;
            // make the residue as input signal to the next EMD iteration
            // Step (6) in Linderhed (2009)
            (*currentSignal) = residueSignal;
        }

        // keep track of number of extrema count to detect divergence
        previousLocalMaximaCount = localMaximaCount;
        previousLocalMinimaCount = localMinimaCount;
    } //----EMD iterations
}
