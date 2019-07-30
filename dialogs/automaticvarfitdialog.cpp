#include "automaticvarfitdialog.h"
#include "ui_automaticvarfitdialog.h"
#include "mainwindow.h"
#include "domain/attribute.h"
#include "domain/cartesiangrid.h"
#include "imagejockey/widgets/ijgridviewerwidget.h"
#include "spectral/spectral.h"
#include "imagejockey/svd/svdfactor.h"

#include <QProgressDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <cassert>
#include <thread>
#include <QTextStream>

AutomaticVarFitDialog::AutomaticVarFitDialog(Attribute *at, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AutomaticVarFitDialog),
    m_at( at ),
    m_cg( nullptr ),
    m_nestedVariogramStructuresParametersForManual( new NestedVariogramStructuresParameters() ),
    m_autoVarFit( at )
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

    onVarmapMethodChanged();

    onNumberOfStructuresChanged( ui->spinNumberOfVariogramStructures->value() );
}

AutomaticVarFitDialog::~AutomaticVarFitDialog()
{
    delete ui;
}

void AutomaticVarFitDialog::onDoWithSAandGD()
{
    //////////////////////////////USER CONFIGURATION////////////////////////////////////
    //.......................Global Parameters................................................
    // Number of parallel execution threads
    unsigned int nThreads = ui->spinNumberOfThreads->value();
    // Number of variogram structures to fit.
    int m = ui->spinNumberOfVariogramStructures->value();
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
    ///////////////////////////////////////////////////////////////////////////////////////////

    m_autoVarFit.processWithSAandGD(
                        nThreads,
                        m,
                        (unsigned)ui->spinSeed->value(),
                        f_Tinitial,
                        f_Tfinal,
                        i_kmax,
                        f_factorSearch,
                        maxNumberOfOptimizationSteps,
                        epsilon,
                        initialAlpha,
                        maxNumberOfAlphaReductionSteps,
                        convergenceCriterion );
}

void AutomaticVarFitDialog::onDoWithLSRS()
{
    //////////////////////////////USER CONFIGURATION////////////////////////////////////
    // Number of parallel execution threads
    unsigned int nThreads = ui->spinNumberOfThreads->value();
    int m = ui->spinNumberOfVariogramStructures->value();
    int maxNumberOfOptimizationSteps = ui->spinMaxStepsLSRS->value();
    // The user-given epsilon (useful for numerical calculus).
    double epsilon = std::pow(10, ui->spinLogEpsilonLSRS->value() );
    int nStartingPoints = ui->spinNumberOfStartingPoints->value(); //number of random starting points in the domain
    int nRestarts = ui->spinNumberOfRestarts->value(); //number of restarts
    ////////////////////////////////////////////////////////////////////////////////////

    m_autoVarFit.processWithLSRS(
            nThreads,
            m,
            (unsigned)ui->spinSeed->value(),
            maxNumberOfOptimizationSteps,
            epsilon,
            nStartingPoints,
            nRestarts
            );
}

void AutomaticVarFitDialog::onDoWithPSO()
{
    //////////////////////////////USER CONFIGURATION////////////////////////////////////
    int m = ui->spinNumberOfVariogramStructures->value();
    int maxNumberOfOptimizationSteps = ui->spinMaxStepsPSO->value();
    // The user-given epsilon (useful for numerical calculus).
    int nParticles = ui->spinNumberOfParticles->value(); //number of wandering particles
    double intertia_weight = ui->dblSpinInertiaWeight->value();
    double acceleration_constant_1 = ui->dblSpinAccelerationConstant1->value();
    double acceleration_constant_2 = ui->dblSpinAccelerationConstant2->value();
    /////////////////////////////////////////////////////////////////////////////////////

    m_autoVarFit.processWithPSO(
            m,
            (unsigned)ui->spinSeed->value(),
            maxNumberOfOptimizationSteps,
            nParticles,
            intertia_weight,
            acceleration_constant_1,
            acceleration_constant_2
            );
}

void AutomaticVarFitDialog::onDoWithGenetic()
{
    //////////////////////////////USER CONFIGURATION////////////////////////////////////
    int nThreads = ui->spinNumberOfThreads->value();
    int m = ui->spinNumberOfVariogramStructures->value();
    int maxNumberOfGenerations = ui->spinNumberOfGenerations->value();
    uint nPopulationSize = ui->spinPopulationSize->value(); //number of individuals (sets of parameters)
    uint nSelectionSize = ui->spinSelectionSize->value(); //the size of the selection pool (must be < nPopulationSize)
    double probabilityOfCrossOver = ui->dblSpinProbabilityOfCrossover->value();
    uint pointOfCrossover = ui->spinPointOfCrossover->value(); //the index where crossover switches (must be less than the total number of parameters per individual)
    //mutation rate means how many paramaters are expected to change per mutation
    //the probability of any parameter parameter (gene) to be changed is 1/nParameters * mutationRate
    //thus, 1.0 means that one gene will surely be mutated per mutation on average.  Fractionary
    //values are possible. 0.0 means no mutation will take place.
    double mutationRate = ui->dblSpinMutationRate->value();
    //////////////////////////////////////////////////////////////////////////////////////

    m_autoVarFit.processWithGenetic(
            nThreads,
            m,
            (unsigned)ui->spinSeed->value(),
            maxNumberOfGenerations,
            nPopulationSize,
            nSelectionSize,
            probabilityOfCrossOver,
            pointOfCrossover,
            mutationRate);
}

void AutomaticVarFitDialog::onDoWithManual()
{
    int m = ui->spinNumberOfVariogramStructures->value();

    //Compute FFT phase map of input
    spectral::array inputFFTimagPhase = m_autoVarFit.getInputPhaseMap();

    //Compute varmap of input
    spectral::array inputVarmap = m_autoVarFit.computeVarmap();

    //Initialize the optimization domain (boundary conditions) and
    //the sets of variogram paramaters (both linear and structured)
    VariogramParametersDomain domain;
    spectral::array vw;
    spectral::array L_wMin;
    spectral::array L_wMax;
    std::vector< IJVariographicStructure2D > variogramStructures;
    m_autoVarFit.initDomainAndParameters(
                             inputVarmap,
                             m,
                             domain,
                             vw,
                             L_wMin,
                             L_wMax,
                             variogramStructures );

    //Read the variogram model manually entered by the user
    for( int iStructure = 0; iStructure < m; ++iStructure ){
            variogramStructures[iStructure].setParameter( 0,
                               m_nestedVariogramStructuresParametersForManual->getSemiMajorAxis( iStructure ) );
            variogramStructures[iStructure].setParameter( 1,
                               m_nestedVariogramStructuresParametersForManual->getSemiMinorSemiMajorAxesRatio( iStructure ) );
            variogramStructures[iStructure].setParameter( 2,
                               m_nestedVariogramStructuresParametersForManual->getAzimuthAsRadians( iStructure ) );
            variogramStructures[iStructure].setParameter( 3,
                               m_nestedVariogramStructuresParametersForManual->getContribution( iStructure ) );
     }

    // Display the results in a window.
    m_autoVarFit.displayResults( variogramStructures, inputFFTimagPhase, inputVarmap, false );
}

void AutomaticVarFitDialog::onVarmapMethodChanged()
{
    // display input variable's varmap
    {
        //configure the fast varmap computing method
        if( ui->cmbVarmapMethod->currentIndex() == 0)
            m_autoVarFit.setFastVarmapMethod( FastVarmapMethod::VARMAP_WITH_FIM );
        else
            m_autoVarFit.setFastVarmapMethod( FastVarmapMethod::VARMAP_WITH_SPECTRAL );

        //compute varmap (output will go to temp)
        spectral::array temp = m_autoVarFit.computeVarmap();
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

void AutomaticVarFitDialog::onNumberOfStructuresChanged(int number)
{
    m_nestedVariogramStructuresParametersForManual->setNumberOfNestedStructures( number );
    ui->layoutFieldsForManual->addWidget( m_nestedVariogramStructuresParametersForManual->getEditorWidget() );
}

void AutomaticVarFitDialog::onRunExperiments()
{
    //open a new file for output
    QFile outputFile( QString("C:\\Users\\ur5m\\Desktop\\experiments.txt") );
    outputFile.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&outputFile);

    int experimentNumber = 1;
    for( double tInitial = 20; tInitial <= 20000; tInitial += 2000 )
        for( double tFinal = 20; tFinal <= 10000 && tFinal < tInitial; tFinal += 1000 )
            for( int nSteps = 100; nSteps <= 5000; nSteps += 500 )
                for( double hopFactor = 0.01; hopFactor <= 1.000 ; hopFactor += 0.1 )
                {
                    std::vector< IJVariographicStructure2D > model =
                                  m_autoVarFit.processWithSAandGD(
                                                     8,
                                                     4,
                                                     131313,
                                                     tInitial,
                                                     tFinal,
                                                     nSteps,
                                                     hopFactor,
                                                     20,
                                                     0.000001,
                                                     1.0,
                                                     60,
                                                     0.000001,
                                                     false);
                    out << "t0=" << tInitial
                              << " t1=" << tFinal
                              << " nSteps=" << nSteps
                              << " hop=" << hopFactor
                              << " F=" << m_autoVarFit.evaluateModel( model )
                              << '\n';
                    out.flush();
                    std::cout << experimentNumber << std::endl;
                    ++experimentNumber;
                }

    //closes the output file
    outputFile.close();
}
