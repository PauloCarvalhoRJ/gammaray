#include "automaticvarfitdialog.h"
#include "ui_automaticvarfitdialog.h"
#include "domain/attribute.h"
#include "domain/file.h"
#include "domain/cartesiangrid.h"
#include "domain/application.h"
#include "imagejockey/widgets/ijgridviewerwidget.h"
#include "imagejockey/svd/svdfactor.h"
#include "imagejockey/imagejockeyutils.h"
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
           const int m ) const  {
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
    mapFromTheoreticalVariogramModel = computeFIM( theoreticalVariographicSurface, inputFFTimagPhase );

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

spectral::array AutomaticVarFitDialog::getInputPhaseMap() const
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

    //Initialize the optimization domain (boundary conditions) and
    //the sets of variogram paramaters (both linear and structured)
    VariogramParametersDomain domain;
    spectral::array vw;
    spectral::array L_wMin;
    spectral::array L_wMax;
    std::vector< IJVariographicStructure2D > variogramStructures;
    initDomainAndParameters( inputVarmap,
                             m,
                             domain,
                             vw,
                             L_wMin,
                             L_wMax,
                             variogramStructures );

    //-------------------------------------------------------------------------------------------------------------
    //-------------------------SIMULATED ANNEALING TO INITIALIZE THE PARAMETERS [w] NEAR A GLOBAL MINIMUM------------
    //---------------------------------------------------------------------------------------------------------------
    {

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
        progressDialog.setRange(0, i_kmax);
        progressDialog.setValue( 0 );
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
            progressDialog.setValue( k );
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
    progressDialog.setRange(0, maxNumberOfOptimizationSteps );
    progressDialog.show();
    progressDialog.setValue( 0 );
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

        if( ! ( iOptStep % 10) ) { //to avoid excess calls to processEvents.
            progressDialog.setValue( iOptStep );
            QCoreApplication::processEvents();
        }
    } //--------GD's main loop-------
    progressDialog.hide();

    //Read the optimized variogram model parameters back to the variographic structures
    for( int i = 0, iStructure = 0; iStructure < m; ++iStructure )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            variogramStructures[iStructure].setParameter( iPar, vw[i] );

    // Display the results in a window.
    displayResults( variogramStructures, inputFFTimagPhase, inputVarmap );
}

void AutomaticVarFitDialog::onDoWithLSRS()
{
    //get user configuration
    int m = ui->spinNumberOfVariogramStructures->value();
    int maxNumberOfOptimizationSteps = ui->spinMaxStepsLSRS->value();
    // The user-given epsilon (useful for numerical calculus).
    double epsilon = std::pow(10, ui->spinLogEpsilonLSRS->value() );
    int nStartingPoints = ui->spinNumberOfStartingPoints->value(); //number of random starting points in the domain
    int nRestarts = ui->spinNumberOfRestarts->value(); //number of restarts

    //Intialize the random number generator with the same seed
    std::srand ((unsigned)ui->spinSeed->value());

    // Get the data objects.
    IJAbstractCartesianGrid* inputGrid = m_cg;
    IJAbstractVariable* variable = m_at;

    // Get the grid's dimensions.
    unsigned int nI = inputGrid->getNI();
    unsigned int nJ = inputGrid->getNJ();
    unsigned int nK = inputGrid->getNK();

    // Fetch data from the data source.
    inputGrid->dataWillBeRequested();

    //================================== PREPARE DATA ==========================

    // Get the input data as a spectral::array object
    spectral::arrayPtr inputData( inputGrid->createSpectralArray( variable->getIndexInParentGrid() ) );

    // Compute FFT phase map of input
    spectral::array inputFFTimagPhase = getInputPhaseMap();

    // Compute input's varmap
    spectral::array inputVarmap = computeVarmap();

    //Initialize the optimization domain (boundary conditions) and
    //the sets of variogram paramaters (both linear and structured)
    VariogramParametersDomain domain;
    spectral::array vw;
    spectral::array L_wMin;
    spectral::array L_wMax;
    std::vector< IJVariographicStructure2D > variogramStructures;
    initDomainAndParameters( inputVarmap,
                             m,
                             domain,
                             vw,
                             L_wMin,
                             L_wMax,
                             variogramStructures );

    //-------------------------------------------------------------------------------------------------------------
    //------------------------ THE MODIFIED LINE SEARCH ALGORITHM AS PROPOSED BY Grosan and Abraham (2009)---------
    //---------------------------A Novel Global Optimization Technique for High Dimensional Functions--------------
    //-------------------------------------------------------------------------------------------------------------
    double deltaAxis         = domain.max.range        - domain.min.range;
    double deltaRatio        = domain.max.rangeRatio   - domain.min.rangeRatio;
    double deltaAzimuth      = domain.max.azimuth      - domain.min.azimuth;
    double deltaContribution = domain.max.contribution - domain.min.contribution;

    QProgressDialog progressDialog;
    progressDialog.setRange(0, nRestarts * maxNumberOfOptimizationSteps );
    progressDialog.show();
    progressDialog.setValue( 0 );
    progressDialog.setLabelText("Line Search with Restart in progress...");

    //the line search restarting loop
    spectral::array vw_bestSolution( (spectral::index)( m * IJVariographicStructure2D::getNumberOfParameters() ) );
    for( int t = 0; t < nRestarts; ++t){

        //generate sarting points randomly within the domain
        // each starting point is a potential solution (set of parameters)
        std::vector< spectral::array > startingPoints;
        for( int iSP = 0; iSP < nStartingPoints; ++iSP ){
            spectral::array vw_StartingPoint( (spectral::index)( m * IJVariographicStructure2D::getNumberOfParameters() ) );
            for( int i = 0; i < vw_StartingPoint.size(); ++i ){
                double LO = L_wMin[i];
                double HI = L_wMax[i];
                vw_StartingPoint[i] = LO + std::rand() / (RAND_MAX/(HI-LO));
            }
            startingPoints.push_back( vw_StartingPoint );
        }

        //lambda to define the step as a function of iteration number (the alpha-k in Grosan and Abraham (2009))
        //first iteration must be 1.
        auto alpha_k = [=](int k) { return 2.0 + 3.0 / std::pow(2, k*k + 1); };

        //----------------loop of line search algorithm----------------
        double fOfBestSolution = std::numeric_limits<double>::max();
        //for each step
        for( int k = 1; k <= maxNumberOfOptimizationSteps; ++k ){
            //for each starting point (in the parameter space).
            for( int i = 0; i < nStartingPoints; ++i ){
                //make a candidate point with a vector from the current point.
                spectral::array vw_candidate( (spectral::index)( m * IJVariographicStructure2D::getNumberOfParameters() ) );
                for( int j = 0; j < vw.size(); ++j ){
                    double p_k = -1.0 + std::rand() / ( RAND_MAX / 2.0);//author suggests -1 or drawn from [0.0 1.0] for best results

                    double delta = 0.0;
                    switch( j % IJVariographicStructure2D::getNumberOfParameters() ){
                    case 0: delta = deltaAxis;         break;
                    case 1: delta = deltaRatio;        break;
                    case 2: delta = deltaAzimuth;      break;
                    case 3: delta = deltaContribution; break;
                    }

                    vw_candidate[j] = startingPoints[i][j] + p_k * delta * alpha_k( k );
                    if( vw_candidate[j] > L_wMax[j] )
                        vw_candidate[j] = L_wMax[j];
                    if( vw_candidate[j] < L_wMin[j] )
                        vw_candidate[j] = L_wMin[j];

                }
                //evaluate the objective function for the current point and for the candidate point
                double fCurrent   = objectiveFunction( *inputGrid, *inputData, startingPoints[i], m );
                double fCandidate = objectiveFunction( *inputGrid, *inputData, vw_candidate,      m );
                //if the candidate point improves the objective function...
                if( fCandidate < fCurrent ){
                    //...make it the current point.
                    startingPoints[i] = vw_candidate;
                    //keep track of the best solution
                    if( fCandidate < fOfBestSolution ){
                        fOfBestSolution = fCandidate;
                        vw_bestSolution = vw_candidate;
                    }
                }
            } //for each starting point

            progressDialog.setValue( t * maxNumberOfOptimizationSteps + k );
            QApplication::processEvents(); // let Qt update the UI

        } // search for best solution
        //---------------------------------------------------------------------------

        //for each parameter of the best solution
        for( int iParameter = 0; iParameter < vw.size(); ++iParameter ){
            //Make a set of parameters slightly shifted to the right (more positive) along one parameter.
            spectral::array vwFromRight = vw_bestSolution;
            vwFromRight(iParameter) = vw_bestSolution(iParameter) + epsilon;
            //Make a set of parameters slightly shifted to the left (more negative) along one parameter.
            spectral::array vwFromLeft = vw_bestSolution;
            vwFromLeft(iParameter) = vw_bestSolution(iParameter) - epsilon;
            //compute the partial derivative along one parameter
            double partialDerivative =  ( objectiveFunction( *inputGrid, *inputData, vwFromRight, m )
                                          -
                                          objectiveFunction( *inputGrid, *inputData, vwFromLeft, m ))
                                          /
                                          ( 2 * epsilon );
            //update the domain limits depending on the partial derivative result
            //this usually reduces the size of the domain so the next set of starting
            //points have a higher probability to be drawn near a global optimum.
            if( partialDerivative > 0 )
                L_wMax[ iParameter ] = vw_bestSolution[ iParameter ];
            else if( partialDerivative < 0 )
                L_wMin[ iParameter ] = vw_bestSolution[ iParameter ];
        } // reduce the domain to a smaller hyper volume around the suspected optimum

    } //restart loop
    progressDialog.hide();

    //Read the optimized variogram model parameters back to the variographic structures
    for( int i = 0, iStructure = 0; iStructure < m; ++iStructure )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            variogramStructures[iStructure].setParameter( iPar, vw_bestSolution[i] );

    // Display the results in a window.
    displayResults( variogramStructures, inputFFTimagPhase, inputVarmap );
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
    svdad->show();
}

spectral::array AutomaticVarFitDialog::computeFIM( const spectral::array &gridWithVariographicStructure,
                                                   const spectral::array &gridWithFFTphases ) const
{
    std::unique_lock<std::mutex> FFTWlock ( myMutexFFTW, std::defer_lock );

    //get grid dimensions
    size_t nI = gridWithVariographicStructure.M();
    size_t nJ = gridWithVariographicStructure.N();
    size_t nK = gridWithVariographicStructure.K();

    //prepare the result
    spectral::array result( nI, nJ, nK, 0.0 );

    //compute FFT of the variographic surface (into polar form)
    spectral::complex_array variographicSurfaceFFT( nI, nJ, nK );
    spectral::array tmp = spectral::shiftByHalf( gridWithVariographicStructure );
    FFTWlock.lock();                                  //
    spectral::foward( variographicSurfaceFFT, tmp );  // FFTW crashes when called concurrently
    FFTWlock.unlock();                                //
    spectral::complex_array variographicSurfaceFFTpolar = spectral::to_polar_form( variographicSurfaceFFT );
    spectral::array variographicSurfaceFFTamplitudes = spectral::real( variographicSurfaceFFTpolar );

    //get the square root of the amplitudes of the varmap FFT
    spectral::array variographicSurfaceFFTamplitudesSQRT = variographicSurfaceFFTamplitudes.sqrt();

    //convert sqrt(varmap) and the phases of the input to rectangular form
    spectral::complex_array mapFFTpolar = spectral::to_complex_array(
                                                   variographicSurfaceFFTamplitudesSQRT,
                                                   gridWithFFTphases
                                                );
    spectral::complex_array mapFFT = spectral::to_rectangular_form( mapFFTpolar );

    //compute the reverse FFT to get "factorial kriging"
    FFTWlock.lock();                      //
    spectral::backward( result, mapFFT ); // FFTW crashes when called concurrently
    FFTWlock.unlock();                    //

    //fftw3's reverse FFT requires that the values of output be divided by the number of cells
    result = result / (double)( nI * nJ * nK );

    //return the result
    return result;
}

void AutomaticVarFitDialog::displayResults( const std::vector<IJVariographicStructure2D> &variogramStructures,
                                            const spectral::array& fftPhaseMapOfInput,
                                            const spectral::array& varmapOfInput )
{
    int m = variogramStructures.size();
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
        variogramStructures[iStructure].addContributionToModelGrid( *m_cg,
                                                                     oneStructureVarmap,
                                                                     IJVariogramPermissiveModel::SPHERIC,
                                                                     true );

        //collect the theoretical varmap for display
        //oneStructureVarmap = oneStructureVarmap.max() - oneStructureVarmap; // correlogram -> variogram
        //display inverted so it appears with 0.0 at center (h=0)
        maps.push_back( oneStructureVarmap.max() - oneStructureVarmap );
        titles.push_back( QString( "Structure " + QString::number( iStructure ) ).toStdString() );
        shiftFlags.push_back( false );

        //compute FIM to obtain the map from a variographic structure
        spectral::array oneStructure( nI, nJ, nK, 0.0 );
        oneStructure = computeFIM( oneStructureVarmap, fftPhaseMapOfInput );

        //collect the "FK factor"
        maps.push_back( oneStructure );
        titles.push_back( QString( "Map " + QString::number( iStructure ) ).toStdString() );
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
    maps.push_back( varmapOfInput );
    titles.push_back( QString( "Varmap of input" ).toStdString() );
    shiftFlags.push_back( false );

    // Prepare the display experimental - model
    spectral::array diff = varmapOfInput - variograficSurface;
    maps.push_back( diff );
    titles.push_back( QString( "Difference" ).toStdString() );
    shiftFlags.push_back( false );

    // Display all the grids in a dialog
    displayGrids( maps, titles, shiftFlags );
}

void AutomaticVarFitDialog::initDomainAndParameters( const spectral::array& inputVarmap,
                                                     int m,
                                                     VariogramParametersDomain &domain,
                                                     spectral::array &vw,
                                                     spectral::array &L_wMin,
                                                     spectral::array &L_wMax,
                                                     std::vector<IJVariographicStructure2D>& variogramStructures) const
{
    //define the domain
    double minCellSize = std::min( m_cg->getCellSizeI(), m_cg->getCellSizeJ() );
    double minAxis         = minCellSize              ; double maxAxis         = m_cg->getDiagonalLength() / 2.0;
    double minRatio        = 0.001                    ; double maxRatio        = 1.0;
    double minAzimuth      = 0.0                      ; double maxAzimuth      = ImageJockeyUtils::PI;
    double minContribution = inputVarmap.max() / 100.0; double maxContribution = inputVarmap.max();

    //create the nested structures wanted by the user
    //the parameters are initialized near in the center of the domain
    for( int i = 0; i < m; ++i )
        variogramStructures.push_back( IJVariographicStructure2D (  ( maxAxis         + minAxis         ) / 2.0,
                                                                    ( maxRatio        + minRatio        ) / 2.0,
                                                                    ( minAzimuth      + maxAzimuth      ) / 2.0,
                                                                    ( maxContribution - minContribution ) / m ) );

    //Initialize the vector of all variographic parameters [w]=[axis0,ratio0,az0,cc0,axis1,ratio1,...]
    //this vector is used in optimization steps .
    // the starting values are not particuarly important.
    vw = spectral::array( (spectral::index)( m * IJVariographicStructure2D::getNumberOfParameters() ) );
    for( int iLinearIndex = 0, iStructure = 0; iStructure < m; ++iStructure )
        for( int iIndexInStructure = 0; iIndexInStructure < IJVariographicStructure2D::getNumberOfParameters(); ++iIndexInStructure, ++iLinearIndex )
            vw[iLinearIndex] = variogramStructures[iStructure].getParameter( iIndexInStructure );

    //Minimum value allowed for the parameters w
    //(see min* variables further up). DOMAIN CONSTRAINT
    L_wMin = spectral::array( vw.size(), 0.0 );
    for(int i = 0, iStructure = 0; iStructure < m; ++iStructure )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            switch( iPar ){
            case 0: L_wMin[i] = minAxis;         break;
            case 1: L_wMin[i] = minRatio;        break;
            case 2: L_wMin[i] = minAzimuth;      break;
            case 3: L_wMin[i] = minContribution; break;
            }

    //Maximum value allowed for the parameters w
    //(see max* variables further up). DOMAIN CONSTRAINT
    L_wMax = spectral::array( vw.size(), 1.0 );
    for(int i = 0, iStructure = 0; iStructure < m; ++iStructure )
        for( int iPar = 0; iPar < IJVariographicStructure2D::getNumberOfParameters(); ++iPar, ++i )
            switch( iPar ){
            case 0: L_wMax[i] = maxAxis;         break;
            case 1: L_wMax[i] = maxRatio;        break;
            case 2: L_wMax[i] = maxAzimuth;      break;
            case 3: L_wMax[i] = maxContribution; break;
            }

    // Return the domain
    domain.min.range        = minAxis;
    domain.min.rangeRatio   = minRatio;
    domain.min.azimuth      = minAzimuth;
    domain.min.contribution = minContribution;
    domain.max.range        = maxAxis;
    domain.max.rangeRatio   = maxRatio;
    domain.max.azimuth      = maxAzimuth;
    domain.max.contribution = maxContribution;
}

