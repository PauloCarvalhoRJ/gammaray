#include "automaticvarfitdialog.h"
#include "ui_automaticvarfitdialog.h"
#include "domain/attribute.h"
#include "domain/file.h"
#include "domain/cartesiangrid.h"
#include "domain/application.h"
#include "imagejockey/widgets/ijgridviewerwidget.h"
#include "imagejockey/svd/svdfactor.h"
#include "imagejockey/imagejockeyutils.h"
#include "imagejockey/ijvariographicmodel2d.h"
#include "imagejockey/svd/svdfactortree.h"
#include "imagejockey/svd/svdanalysisdialog.h"

#include <QProgressDialog>
#include <cassert>
#include <thread>
#include <mutex>

/** This is a mutex to restrict access to the FFTW routines from
 * multiple threads.  Some of its routines are not thread safe.*/

std::mutex myMutexFFTW; //ATTENTION NAME CLASH: there is a variable called mutexFFTW defined somewhere out there.

/**
 * The code for multithreaded gradient vector calculation for objective function.
 * @param vw Linear array with all the variographic parameters: [axis0,ratio0,az0,cc0,axis1,ratio1,...]
 * @param parameterIndexBin Vector with the indexes of  the variographic parameters this task can work with.
 * @param epsilon The "small value" used to compute a derivative numerically.
 * @param gridWithGeometry The grid object with the target grid geometry.
 * @param gridData The grid data (data count must be consistent with the grid geometry of gridWithGeometry).
 * @param m The number of nested variogram structures.
 * @param autoVarFitDlgRef A reference to the automaitic variogram fitting dialog to call its objective function.
 * @param gradient a linear array containing the partial derivatives with respect to each parameter.
 */
void taskOnePartialDerivative (
                               const spectral::array& vw,
                               const std::vector< int >& parameterIndexBin,
                               const double epsilon,
                               IJAbstractCartesianGrid* gridWithGeometry,
                               const spectral::array& gridData,
                               const int m,
                               AutomaticVarFitDialog* autoVarFitDlgRef,
                               spectral::array* gradient //output object: for some reason, the thread object constructor does not compile with non-const references.
                               ){
    std::vector< int >::const_iterator it = parameterIndexBin.cbegin();
    for(; it != parameterIndexBin.cend(); ++it ){
        int iParameter = *it;
        //Make a set of parameters slightly shifted to the right (more positive) along one parameter.
        spectral::array vwFromRight( vw );
        vwFromRight(iParameter) = vwFromRight(iParameter) + epsilon;
        //Make a set of parameters slightly shifted to the left (more negative) along one parameter.
        spectral::array vwFromLeft( vw );
        vwFromLeft(iParameter) = vwFromLeft(iParameter) - epsilon;
        //Compute (numerically) the partial derivative with respect to one parameter.
        (*gradient)(iParameter) = ( autoVarFitDlgRef->objectiveFunction( *gridWithGeometry, gridData, vwFromRight, m )
                                     -
                                    autoVarFitDlgRef->objectiveFunction( *gridWithGeometry, gridData, vwFromLeft , m ))
                                     /
                                   ( 2 * epsilon );
    }
}


AutomaticVarFitDialog::AutomaticVarFitDialog(Attribute *at, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AutomaticVarFitDialog),
    m_at( at )
{
    assert( at && "AutomaticVarFitDialog::AutomaticVarFitDialog(): attribute cannot be null.");

    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle( "Automatic variogram model fitting" );

    //display the selected input data names
    File* dataFile = at->getContainingFile();
    ui->lblFileAttribute->setText( dataFile->getName() + "/" + at->getName() );

    //get the number of threads from logical CPUs
    ui->spinNumberOfThreads->setValue( (int)std::thread::hardware_concurrency() );

    // these pointers will be managed by Qt
    m_gridViewerInput = new IJGridViewerWidget( true, false, false );
    m_gridViewerVarmap = new IJGridViewerWidget( true, false, false );
    ui->tabInputAndVarmap->layout()->addWidget( m_gridViewerInput );
    ui->tabInputAndVarmap->layout()->addWidget( m_gridViewerVarmap );

    // get the input data's grid
    m_cg = dynamic_cast<CartesianGrid*>( dataFile );
    assert( m_cg && "AutomaticVarFitDialog::AutomaticVarFitDialog(): only attributes from CartesianGrids can be used.");

    // display input variable
    {
        //Get input data as a raw data array
        spectral::arrayPtr inputData( m_cg->createSpectralArray( m_at->getAttributeGEOEASgivenIndex()-1 ) );
        //make a local copy (will be moved to inside of a SVDFacor object)
        spectral::array temp( *inputData );
        //make a SVDFactor object so we can reuse IJGridViewerWidget to display gridded data
        SVDFactor* gridData = new SVDFactor( std::move(temp), 1, 0.42,
                                         m_cg->getOriginX(),
                                         m_cg->getOriginY(),
                                         m_cg->getOriginZ(),
                                         m_cg->getCellSizeI(),
                                         m_cg->getCellSizeJ(),
                                         m_cg->getCellSizeK(),
                                         0.0 );
        //set color scale label
        gridData->setCustomName( m_at->getName() );
        //display data
        m_gridViewerInput->setFactor( gridData );
    }

    // display input variable's varmap
    {
        //compute varmap (output will go to temp)
        spectral::array temp = computeVarmap();
        //make a SVDFactor object so we can reuse IJGridViewerWidget to display gridded data
        SVDFactor* gridData = new SVDFactor( std::move(temp), 1, 0.42,
                                         -m_cg->getNI() / 2 * m_cg->getCellSizeI(),
                                         -m_cg->getNJ() / 2 * m_cg->getCellSizeJ(),
                                         -m_cg->getNK() / 2 * m_cg->getCellSizeK(),
                                         m_cg->getCellSizeI(),
                                         m_cg->getCellSizeJ(),
                                         m_cg->getCellSizeK(),
                                         0.0 );
        //set color scale label
        gridData->setCustomName( "Varmap" );
        //display data
        m_gridViewerVarmap->setFactor( gridData );
    }
}

AutomaticVarFitDialog::~AutomaticVarFitDialog()
{
    delete ui;
}

spectral::array AutomaticVarFitDialog::computeVarmap() const
{
    //Get input data as a raw data array
    spectral::arrayPtr inputData( m_cg->createSpectralArray( m_at->getAttributeGEOEASgivenIndex()-1 ) ) ;
    //make a local copy (will be moved to inside of a SVDFacor object)
    spectral::array temp( *inputData );
    //compute varmap (output will go to temp)
    spectral::autocovariance( temp , *inputData, true );
    //put covariance at h=0 in the center of the grid for ease of interpretation
    temp = spectral::shiftByHalf( temp );
    //clips the varmap so the grid matches the input's
    temp = spectral::project( temp, m_cg->getNI(), m_cg->getNJ(), m_cg->getNK() );
    //invert result so the value increases radially from the center at h=0
    temp = temp.max() - temp;
    return temp;
}

spectral::array AutomaticVarFitDialog::generateVariographicSurface(
                                            IJAbstractCartesianGrid& gridWithGeometry,
                                            const spectral::array &vectorOfParameters,
                                            const int m ) const {
    //get grid parameters
    int nI = gridWithGeometry.getNI();
    int nJ = gridWithGeometry.getNJ();
    int nK = gridWithGeometry.getNK();
    //create a grid compatible with the input varmap
    spectral::array variographicSurface( nI, nJ, nK, 0.0 );
    //for each variogram structure
    for( int i = 0, iStructure = 0; iStructure < m; ++iStructure ){
        //create a variographic structure
        IJVariographicStructure2D varEllip(0.0, 0.0, 0.0, 0.0);
        //for each variographic parameter
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i ){
            //set it to the variographic ellipse
            varEllip.setParameter( iPar, vectorOfParameters[i] );
        }
        //make the variographic surface
        varEllip.addContributionToModelGrid( gridWithGeometry,
                                             variographicSurface,
                                             IJVariogramPermissiveModel::SPHERIC,
                                             true );
    }
    return variographicSurface;
}


double AutomaticVarFitDialog::objectiveFunction( IJAbstractCartesianGrid& gridWithGeometry,
           const spectral::array &inputGridData,
           const spectral::array &vectorOfParameters,
           const int m )  {
    std::unique_lock<std::mutex> FFTWlock ( myMutexFFTW, std::defer_lock );

    //get grid parameters
    int nI = gridWithGeometry.getNI();
    int nJ = gridWithGeometry.getNJ();
    int nK = gridWithGeometry.getNK();

    //generate the variogram model surface
    spectral::array theoreticalVariographicSurface = generateVariographicSurface( gridWithGeometry,
                                                                       vectorOfParameters,
                                                                       m );

    //Get input's FFT phase map
    spectral::array inputFFTimagPhase = getInputPhaseMap();

    //Apply the principle of the Fourier Integral Method to obtain what would the map be
    //if it actually had the theoretical variogram model
    spectral::array mapFromTheoreticalVariogramModel( nI, nJ, nK, 0.0 );
    {
        //compute FFT of the theoretical varmap (into polar form)
        spectral::complex_array theoreticalVarMapFFT( nI, nJ, nK );
        spectral::array tmp = spectral::shiftByHalf( theoreticalVariographicSurface );
        FFTWlock.lock();                                //
        spectral::foward( theoreticalVarMapFFT, tmp );  // FFTW crashes when called concurrently
        FFTWlock.unlock();                              //
        spectral::complex_array theoreticalVarMapFFTpolar = spectral::to_polar_form( theoreticalVarMapFFT );
        spectral::array theoreticalVarMapFFTamplitudes = spectral::real( theoreticalVarMapFFTpolar );

        //get the square root of the amplitudes of the varmap FFT
        spectral::array theoreticalVarmapFFTamplitudesSQRT = theoreticalVarMapFFTamplitudes.sqrt();

        //convert sqrt(varmap) and the phases of the input to rectangular form
        spectral::complex_array mapFFTpolar = spectral::to_complex_array(
                                                       theoreticalVarmapFFTamplitudesSQRT,
                                                       inputFFTimagPhase
                                                    );
        spectral::complex_array mapFFT = spectral::to_rectangular_form( mapFFTpolar );

        //compute the reverse FFT to get "factorial kriging"
        FFTWlock.lock();                                                //
        spectral::backward( mapFromTheoreticalVariogramModel, mapFFT ); // FFTW crashes when called concurrently
        FFTWlock.unlock();                                              //

        //fftw3's reverse FFT requires that the values of output be divided by the number of cells
        mapFromTheoreticalVariogramModel = mapFromTheoreticalVariogramModel / (double)( nI * nJ * nK );
    }

    //compute the objective function metric
    double sum = 0.0;
    for( int k = 0; k < nK; ++k )
        for( int j = 0; j < nJ; ++j )
            for( int i = 0; i < nI; ++i )
                sum += std::abs( ( inputGridData(i,j,k) - mapFromTheoreticalVariogramModel(i,j,k) ) );
//    sum /= inputGridData.size();

//    VariographicDecompositionDialog::displayGrid( inputGridData, "input data", false );
//    VariographicDecompositionDialog::displayGrid( mapFromTheoreticalVariogramModel, "FIM", false );

    // Finally, return the objective function value.
    return sum;
}

spectral::array AutomaticVarFitDialog::getInputPhaseMap()
{
    std::unique_lock<std::mutex> FFTWlock ( myMutexFFTW, std::defer_lock );
    spectral::arrayPtr inputGridData( m_cg->createSpectralArray( m_at->getAttributeGEOEASgivenIndex()-1 ) );
    spectral::array tmp( *inputGridData );
    spectral::complex_array inputFFT;
    FFTWlock.lock();                //
    spectral::foward( inputFFT, tmp ); // FFTW crashes when called concurrently
    FFTWlock.unlock();              //
    spectral::complex_array inputFFTpolar = spectral::to_polar_form( inputFFT );
    return spectral::imag( inputFFTpolar );
}

void AutomaticVarFitDialog::onDoWithSAandGD()
{
    /////-----------------GET USER CONFIGURATIONS-----------------------------------------------------------
    //.......................Global Parameters................................................
    // Number of parallel execution threads
    unsigned int nThreads = ui->spinNumberOfThreads->value();
    // Number of variogram structures to fit.
    int m = ui->spinNumberOfVariogramStructures->value();
    // Intialize the random number generator with the same seed
    std::srand ((unsigned)ui->spinSeed->value());
    //...................................Annealing Parameters.................................
    // Intial temperature.
    double f_Tinitial = ui->spinInitialTemperature->value();
    // Final temperature.
    double f_Tfinal = ui->spinFinalTemperature->value();
    // Max number of SA steps.
    int i_kmax = ui->spinMaxStepsSA->value();
    /*Factor used to control the size of the random state “hop”.  For example, if the maximum “hop” must be
     10% of domain size, set 0.1.  Small values (e.g. 0.001) result in slow, but more accurate convergence.
     Large values (e.g. 100.0) covers more space faster, but falls outside the domain are more frequent,
     resulting in more re-searches due to more invalid parameter value penalties. */
    double f_factorSearch = ui->spinMaxHopFactor->value();
    //........................Gradient Descent Parameters.....................................
    // Max number of GD steps
    int maxNumberOfOptimizationSteps = ui->spinMaxSteps->value();
    // User-given epsilon (useful for numerical calculus).
    double epsilon = std::pow(10, ui->spinLogEpsilon->value() );
    // Alpha is the factor by which the gradient value is multiplied
    // a small value prevents overshooting.
    double initialAlpha = ui->spinInitialAlpha->value();
    // Alpha is reduced iteratively until a descent is detected (no overshoot)
    double maxNumberOfAlphaReductionSteps = ui->spinMaxStepsAlphaReduction->value();
    // GD stops after two consecutive steps yield two objective function values whose
    // difference is less than this value.
    double convergenceCriterion = std::pow(10, ui->spinConvergenceCriterion->value() );
    /////----------------END OF USER CONFIGURATIONS---------------------------------------------------------


    //================================== PREPARE OPTIMIZATION =============================================

    // Get the input data as a spectral::array object
    spectral::arrayPtr inputData( m_cg->createSpectralArray( m_at->getAttributeGEOEASgivenIndex() - 1 ) );

    // Get experimental variogram (varmap) to be used for comparison with the variogram model
    // in the objective function.
    spectral::array inputVarmap = computeVarmap();

    //Get input's FFT phase map
    spectral::array inputFFTimagPhase = getInputPhaseMap();

    //.... The paramaters (for each nested structure)
    // Axis: the variographic range along the main axis of the anisotropy ellipsis
    // Ratio: the ratio between semi-minor axis and Axis.
    // Azimuth: the azimuth of Axis.
    // Contribution: the semi-variance contribution of the nested structure

    //define the domain
    double minAxis         = 0.0  ; double maxAxis         = m_cg->getDiagonalLength() / 2.0;
    double minRatio        = 0.001; double maxRatio        = 1.0;
    double minAzimuth      = 0.0  ; double maxAzimuth      = ImageJockeyUtils::PI;
    double minContribution = 0.0  ; double maxContribution = inputVarmap.max();

    //create the nested structures wanted by the user
    //the parameters are initialized near in the center of the domain
    std::vector< IJVariographicStructure2D > variographicEllipses;
    for( int i = 0; i < m; ++i )
        variographicEllipses.push_back( IJVariographicStructure2D ( ( maxAxis         + minAxis         ) / 2.0,
                                                                    ( maxRatio        + minRatio        ) / 2.0,
                                                                    ( minAzimuth      + maxAzimuth      ) / 2.0,
                                                                    ( maxContribution - minContribution ) / 4.0 ) );

    //Initialize the vector of all variographic parameters [w]=[axis0,ratio0,az0,cc0,axis1,ratio1,...]
    //this vector is used in optimization steps (SA and then GD).
    spectral::array vw( (spectral::index)( m * IJVariographicStructure2D::getNumberOfParameters() ) );
    for( int iLinearIndex = 0, iNestedStructure = 0; iNestedStructure < m; ++iNestedStructure )
        for( int iIndexInStructure = 0; iIndexInStructure < IJVariographicStructure2D::getNumberOfParameters(); ++iIndexInStructure, ++iLinearIndex )
            vw[iLinearIndex] = variographicEllipses[iNestedStructure].getParameter( iIndexInStructure );

    //-------------------------------------------------------------------------------------------------------------
    //-------------------------SIMULATED ANNEALING TO INITIALIZE THE PARAMETERS [w] NEAR A GLOBAL MINIMUM------------
    //---------------------------------------------------------------------------------------------------------------
    {

        //Minimum value allowed for the parameters w (see min* variables further up). DOMAIN CONSTRAINT
        spectral::array L_wMin( vw.size(), 0.0 );
        for(int i = 0, iStructure = 0; iStructure < m; ++iStructure )
            for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
                switch( iPar ){
                case 0: L_wMin[i] = minAxis;         break;
                case 1: L_wMin[i] = minRatio;        break;
                case 2: L_wMin[i] = minAzimuth;      break;
                case 3: L_wMin[i] = minContribution; break;
                }

        //Maximum value allowed for the parameters w (see max* variables further up). DOMAIN CONSTRAINT
        spectral::array L_wMax( vw.size(), 1.0 );
        for(int i = 0, iStructure = 0; iStructure < m; ++iStructure )
            for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
                switch( iPar ){
                case 0: L_wMax[i] = maxAxis;         break;
                case 1: L_wMax[i] = maxRatio;        break;
                case 2: L_wMax[i] = maxAzimuth;      break;
                case 3: L_wMax[i] = maxContribution; break;
                }

        /* This lambda returns the current “temperature” of the system.  It yields a log curve that decays as the step number increase.
           The initial temperature plays an important role: curve starting with 5.000 is steeper than another that starts with 1.000.
           This means the the lower the temperature, the more linear the temperature decreases.
           i_stepNumber: the current step number of the annealing process ( 0 = first ). */
        auto temperature = [=](int i_stepNumber) {
                  return f_Tinitial * std::exp( -i_stepNumber / (double)1000 * (1.5 * std::log10( f_Tinitial ) ) );
        };

        /* This lambda returns the probability of acceptance of the energy state for the next iteration.
          This allows acceptance of higher values to break free from local minima.
          f_eCurrent: current energy of the system.
          f_eNew: energy level of the next step.
          f_T: current “temperature” of the system.*/
        auto probAcceptance = [=]( double f_eCurrent, double f_eNewLocal, double f_T ) {
           //If the new state is more energetic, calculates a probability of acceptance
           //which is as high as the current “temperature” of the process.  The “temperature”
           //diminishes with iterations.
           if ( f_eNewLocal > f_eCurrent )
              return ( f_T - f_Tfinal ) / ( f_Tinitial - f_Tfinal );
           //If the new state is less energetic, the probability of acceptance is 100% (natural search for minima).
           else
              return 1.0 - ( f_T - f_Tfinal ) / ( f_Tinitial - f_Tfinal );
        };

        //Get the number of parameters.
        int i_nPar = vw.size();

        //Make a copy of the initial state (parameter set).
        spectral::array L_wCurrent( vw );

        //The parameters variations (maxes - mins)
        spectral::array L_wDelta = L_wMax - L_wMin;

        //Give visual feedback to the user (processing may take a while)
        QProgressDialog progressDialog;
        progressDialog.setRange(0,0);
        progressDialog.show();
        progressDialog.setLabelText("Simulated Annealing in progress...");
        QCoreApplication::processEvents();

        //...................Main annealing loop...................
        double f_eNew = std::numeric_limits<double>::max();
        double f_lowestEnergyFound = std::numeric_limits<double>::max();
        spectral::array L_wOfLowestEnergyFound;
        int k = 0;
        for( ; k < i_kmax; ++k ){
            Application::instance()->logInfo( "Commencing SA step #" + QString::number( k ) );
            //Get current temperature.
            double f_T = temperature( k );
            //Quit if temperature is lower than the minimum annealing temperature.
            if( f_T < f_Tfinal )
                /*break*/;
            //Randomly searches for a neighboring state with respect to current state.
            spectral::array L_wNew(L_wCurrent);
            for( int i = 0; i < i_nPar; ++i ){ //for each parameter
               //Ensures that the values randomly obtained are inside the domain.
               double f_tmp = 0.0;
               while( true ){
                  double LO = L_wCurrent[i] - (f_factorSearch * L_wDelta[i]);
                  double HI = L_wCurrent[i] + (f_factorSearch * L_wDelta[i]);
                  f_tmp = LO + std::rand() / (RAND_MAX/(HI-LO)) ;
                  if ( f_tmp >= L_wMin[i] && f_tmp <= L_wMax[i] )
                     break;
               }
               //Updates the parameter value.
               L_wNew[i] = f_tmp;
            }
            //Computes the “energy” of the current state (set of parameters).
            //The “energy” in this case is how different the image as given the parameters is with respect
            //the data grid, considered the reference image.
            double f_eCurrent = objectiveFunction( *m_cg, *inputData, L_wCurrent, m );

            //Computes the “energy” of the neighboring state.
            f_eNew = objectiveFunction( *m_cg, *inputData, L_wNew, m );
            //Changes states stochastically.  There is a probability of acceptance of a more energetic state so
            //the optimization search starts near the global minimum and is not trapped in local minima (hopefully).
            double f_probMov = probAcceptance( f_eCurrent, f_eNew, f_T );
            if( f_probMov >= ( (double)std::rand() / RAND_MAX ) ) {//draws a value between 0.0 and 1.0
                L_wCurrent = L_wNew; //replaces the current state with the neighboring random state
                 Application::instance()->logInfo("  moved to energy level " + QString::number( f_eNew ));
                //if the energy is the record low, store it, just in case the SA loop ends without converging.
                if( f_eNew < f_lowestEnergyFound ){
                    f_lowestEnergyFound = f_eNew;
                    L_wOfLowestEnergyFound = spectral::array( L_wCurrent );
                }
            }

            //Let Qt repaint the GUI
            QCoreApplication::processEvents();
        } //.........................end of main annealing loop.................

        // Delivers the set of parameters near the global minimum (hopefully) for the Gradient Descent algorithm.
        // The SA loop may end in a higher energy state, so we return the lowest found in that case
        if( k == i_kmax && f_lowestEnergyFound < f_eNew )
            Application::instance()->logInfo( "SA completed by number of steps." );
        else
            Application::instance()->logInfo( "SA completed by reaching the lowest temperature." );
        vw = L_wOfLowestEnergyFound;
        Application::instance()->logInfo( "Using the state of lowest energy found (" + QString::number( f_lowestEnergyFound ) + ")" );
    }

    //---------------------------------------------------------------------------------------------------------
    //-------------------------------------- GRADIENT DESCENT PART --------------------------------------------
    //---------------------------------------------------------------------------------------------------------
    QProgressDialog progressDialog;
    progressDialog.setRange(0,0);
    progressDialog.show();
    progressDialog.setLabelText("Gradient Descent in progress...");
    int iOptStep = 0;
    for( ; iOptStep < maxNumberOfOptimizationSteps; ++iOptStep ){

        Application::instance()->logInfo( "Commencing GD step #" + QString::number( iOptStep ) );

        //Compute the gradient vector of objective function F with the current [w] parameters.
        spectral::array gradient( vw.size() );
        {

            //distribute the parameter indexes among the n-threads
            std::vector<int> parameterIndexBins[nThreads];
            int parameterIndex = 0;
            for( unsigned int iThread = 0; parameterIndex < vw.size(); ++parameterIndex, ++iThread)
                parameterIndexBins[ iThread % nThreads ].push_back( parameterIndex );

            //create and run the partial derivative calculation threads
            std::thread threads[nThreads];
            for( unsigned int iThread = 0; iThread < nThreads; ++iThread){
                threads[iThread] = std::thread( taskOnePartialDerivative,
                                                vw,
                                                parameterIndexBins[iThread],
                                                epsilon,
                                                m_cg,
                                                inputVarmap,
                                                m,
                                                this,
                                                &gradient);
            }

            //wait for the threads to finish.
            for( unsigned int iThread = 0; iThread < nThreads; ++iThread)
                threads[iThread].join();

        }

        //Update the system's parameters according to gradient descent.
        double currentF = std::numeric_limits<double>::max();
        double nextF = 1.0;
        {
            double alpha = initialAlpha;
            //halves alpha until we get a descent (current gradient vector may result in overshooting)
            int iAlphaReductionStep = 0;
            for( ; iAlphaReductionStep < maxNumberOfAlphaReductionSteps; ++iAlphaReductionStep ){
                spectral::array new_vw = vw - gradient * alpha;
                //Impose domain constraints to the parameters.
                for( int i = 0; i < new_vw.size(); ++i){
                    if( new_vw.d_[i] < 0.0 )
                        new_vw.d_[i] = 0.0;
                    if( new_vw.d_[i] > 1.0 )
                        new_vw.d_[i] = 1.0;
                }
                currentF = objectiveFunction( *m_cg, *inputData, vw,     m );
                nextF =    objectiveFunction( *m_cg, *inputData, new_vw, m );
                if( nextF < currentF ){
                    vw = new_vw;
                    break;
                }
                alpha /= 2.0;
            }
            if( iAlphaReductionStep == maxNumberOfAlphaReductionSteps )
                Application::instance()->logWarn( "WARNING: reached maximum alpha reduction steps." );
        }

        //Check the convergence criterion.
        double ratio = currentF / nextF;
        if( ratio  < (1.0 + convergenceCriterion) )
            break;

        Application::instance()->logInfo( "F(k)/F(k+1) ratio: " + QString::number( ratio ) );

        if( ! ( iOptStep % 10) ) //to avoid excess calls to processEvents.
            QCoreApplication::processEvents();
    } //--------GD's main loop-------
    progressDialog.hide();

    //Read the optimized variogram model parameters back to the variographic structures
    for( int i = 0, iStructure = 0; iStructure < m; ++iStructure )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            variographicEllipses[iStructure].setParameter( iPar, vw[i] );

    //=====================================GET RESULTS========================================

    //Apply the principle of the Fourier Integral Method
    //use a variographic map as the magnitudes and the FFT phases of
    //the original data to a reverse FFT in polar form to achieve a
    //Factorial Kriging-like separation
    std::vector< spectral::array > maps;
    std::vector< std::string > titles;
    std::vector< bool > shiftFlags;
    uint nI = m_cg->getNI();
    uint nJ = m_cg->getNJ();
    uint nK = m_cg->getNK();
    for( int iStructure = 0; iStructure < m; ++iStructure ) {
        //compute the theoretical varmap for one structure
        spectral::array oneStructureVarmap( nI, nJ, nK, 0.0 );
        variographicEllipses[iStructure].addContributionToModelGrid( *m_cg,
                                                                     oneStructureVarmap,
                                                                     IJVariogramPermissiveModel::SPHERIC,
                                                                     true );

        //collect the theoretical varmap for display
        oneStructureVarmap = oneStructureVarmap.max() - oneStructureVarmap; // correlogram -> variogram
        maps.push_back( oneStructureVarmap );
        titles.push_back( QString( "Varmap " + QString::number( iStructure ) ).toStdString() );
        shiftFlags.push_back( false );

        //compute FFT of the theoretical varmap (into polar form)
        spectral::complex_array oneStructureVarmapFFT( nI, nJ, nK );
        spectral::array tmp = spectral::shiftByHalf( oneStructureVarmap );
        spectral::foward( oneStructureVarmapFFT, tmp );
        spectral::complex_array oneStructureVarmapFFTpolar = spectral::to_polar_form( oneStructureVarmapFFT );
        spectral::array oneStructureVarmapFFTamplitudes = spectral::real( oneStructureVarmapFFTpolar );

        //get the square root of the amplitudes of the varmap FFT
        spectral::array oneStructureVarmapFFTamplitudesSQRT = oneStructureVarmapFFTamplitudes.sqrt();

        //convert sqrt(varmap) and the phases of the input to rectangular form
        spectral::complex_array oneStructureFFTpolar = spectral::to_complex_array(
                                                       oneStructureVarmapFFTamplitudesSQRT,
                                                       inputFFTimagPhase
                                                    );
        spectral::complex_array oneStructureFFT = spectral::to_rectangular_form( oneStructureFFTpolar );

        //compute the reverse FFT to get the map corresponding to one structure
        spectral::array oneStructure( nI, nJ, nK, 0.0 );
        spectral::backward( oneStructure, oneStructureFFT );

        //fftw3's reverse FFT requires that the values of output be divided by the number of cells
        oneStructure = oneStructure / (double)( nI * nJ * nK );

        maps.push_back( oneStructure );
        titles.push_back( QString( "Structure " + QString::number( iStructure ) ).toStdString() );
        shiftFlags.push_back( false );
    }

    // Prepare the display the variogram model surface
    spectral::array variograficSurface( nI, nJ, nK, 0.0 );
    for( spectral::array& oneStructure : maps ){
        variograficSurface += oneStructure;
    }
    maps.push_back( variograficSurface );
    titles.push_back( QString( "Variogram model surface" ).toStdString() );
    shiftFlags.push_back( false );

    // Prepare the display the experimental varmap of the input
    maps.push_back( inputVarmap );
    titles.push_back( QString( "Varmap of input" ).toStdString() );
    shiftFlags.push_back( false );

    // Prepare the display experimental - model
    spectral::array diff = inputVarmap - variograficSurface;
    maps.push_back( diff );
    titles.push_back( QString( "Difference" ).toStdString() );
    shiftFlags.push_back( false );

    // Display all the grids in a dialog
    displayGrids( maps, titles, shiftFlags );
}

void AutomaticVarFitDialog::onDoWithLSRS()
{

}

void AutomaticVarFitDialog::onDoWithPSO()
{

}

void AutomaticVarFitDialog::onDoWithGenetic()
{

}

void AutomaticVarFitDialog::displayGrids(const std::vector<spectral::array> &grids,
                                         const std::vector<std::string> &titles,
                                         const std::vector<bool> & shiftByHalves)
{
    //Create the structure to store the geological factors
    SVDFactorTree * factorTree = new SVDFactorTree( 0.0 ); //the split factor of 0.0 has no special meaning here
    //Populate the factor container with the geological factors.
    std::vector< spectral::array >::const_iterator it = grids.begin();
    std::vector< std::string >::const_iterator itTitles = titles.begin();
    std::vector< bool >::const_iterator itShiftByHalves = shiftByHalves.begin();
    for(int i = 1; it != grids.end(); ++it, ++i, ++itTitles, ++itShiftByHalves){
        //make a local copy of the geological factor data
        spectral::array geoFactorDataCopy;
        if( *itShiftByHalves )
            geoFactorDataCopy = spectral::shiftByHalf( *it );
        else
            geoFactorDataCopy = spectral::array( *it );
        //Create a displayble object from the geological factor data
        //This pointer will be managed by the SVDFactorTree object.
        SVDFactor* geoFactor = new SVDFactor( std::move(geoFactorDataCopy), i, 1/(grids.size()),
                                           0, 0, 0, 1, 1, 1, 0.0);
        //Declare it as a geological factor (decomposable, not fundamental)
        geoFactor->setType( SVDFactorType::GEOLOGICAL );
        geoFactor->setCustomName( QString( (*itTitles).c_str() ) );
        //add the displayable object to the factor tree (container)
        factorTree->addFirstLevelFactor( geoFactor );
    }
    //use the SVD analysis dialog to display the geological factors.
    //NOTE: do not use heap to allocate the dialog, unless you remove the Qt::WA_DeleteOnClose behavior of the dialog.
    SVDAnalysisDialog* svdad = new SVDAnalysisDialog( this );
    svdad->setTree( factorTree );
    svdad->setDeleteTreeOnClose( true ); //the three and all data it contains will be deleted on dialog close
    connect( svdad, SIGNAL(sumOfFactorsComputed(spectral::array*)),
             this, SLOT(onSumOfFactorsWasComputed(spectral::array*)) );
    svdad->exec(); //open the dialog modally
}

