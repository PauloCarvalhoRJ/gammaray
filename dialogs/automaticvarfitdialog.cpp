#include "automaticvarfitdialog.h"
#include "ui_automaticvarfitdialog.h"
#include "mainwindow.h"
#include "domain/attribute.h"
#include "domain/cartesiangrid.h"
#include "domain/application.h"
#include "imagejockey/widgets/ijgridviewerwidget.h"
#include "spectral/spectral.h"
#include "imagejockey/svd/svdfactor.h"
#include "dialogs/automaticvarfitexperimentsdialog.h"
#include "dialogs/emptydialog.h"

#include <QProgressDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <cassert>
#include <thread>
#include <QTextStream>
#include <QChart>
#include <QLineSeries>
#include <QChartView>
#include <QValueAxis>

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

    ui->btnRunExperiments->setEnabled( false );

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
    AutomaticVarFitExperimentsDialog expd;

    int tabIndex = ui->tabMethods->currentIndex();

    //configure the parameter ranging dialog's UI
    {
        expd.setAgorithmLabel( ui->tabMethods->tabText( tabIndex ) );
        switch ( tabIndex ) {
        case 1: //SA+GD algorithm
            expd.setParameterList( {"seed",
                                    "initial temperature",
                                    "final temperature",
                                    "max. 'hop' factor"} );
            expd.setFromSpinBoxConfigs( { { 1000, 1000000, 10000 },
                                          { 20.0, 20000.0, 1000.0 },
                                          { 2.0, 20000.0, 1000.0 },
                                          { .01, 1.0, .1 }} );
            expd.setToSpinBoxConfigs( { { 1000, 1000000, 10000 },
                                          { 20.0, 20000.0, 1000.0 },
                                          { 2.0, 20000.0, 1000.0 },
                                          { .01, 1.0, .1 }} );
            expd.setStepsSpinBoxConfigs({ { 2, 50, 1 },
                                          { 2, 50, 1 },
                                          { 2, 50, 1 },
                                          { 2, 50, 1 }} );
            break;
        case 2: //LSRS algorithm
            expd.setParameterList( {"seed",
                                    "number of lines"} );
            expd.setFromSpinBoxConfigs( { { 1000, 1000000, 10000 },
                                          { 2, 900, 10 }} );
            expd.setToSpinBoxConfigs( { { 1000, 1000000, 10000 },
                                          { 10, 1000, 10 }} );
            expd.setStepsSpinBoxConfigs({ { 2, 50, 1 },
                                          { 2, 50, 1 }} );
            break;
        case 3: //PSO algorithm
            expd.setParameterList( {"seed",
                                    "number of particles",
                                    "inertia",
                                    "acceleration constant 1",
                                    "acceleration constant 2"} );
            expd.setFromSpinBoxConfigs( { { 1000, 1000000, 10000 },
                                          { 5, 1000, 10 },
                                          { 0.01, 20.0, 0.05 },
                                          { 0.01, 20.0, 0.05 },
                                          { 0.01, 20.0, 0.05 }} );
            expd.setToSpinBoxConfigs( { { 1000, 1000000, 10000 },
                                        { 5, 1000, 10 },
                                        { 0.01, 20.0, 0.05 },
                                        { 0.01, 20.0, 0.05 },
                                        { 0.01, 20.0, 0.05 }} );
            expd.setStepsSpinBoxConfigs({ { 2, 50, 1 },
                                          { 2, 50, 1 },
                                          { 2, 50, 1 },
                                          { 2, 50, 1 },
                                          { 2, 50, 1 }} );
            break;
        case 4: //Genetic algorithm
            expd.setParameterList( {"seed",
                                    "population size",
                                    "selection size",
                                    "probability of crossover",
                                    "point of crossover",
                                    "mutation rate"} );
            expd.setFromSpinBoxConfigs( { { 1000, 1000000, 10000 },
                                          { 6, 1000, 10 },
                                          { 2, 1000, 10 },
                                          { 0.01, 1.0, 0.05 },
                                          { 1, 80, 1 },
                                          { 0.01, 1.0, 0.05 }} );
            expd.setToSpinBoxConfigs( { { 1000, 1000000, 10000 },
                                          { 6, 1000, 10 },
                                          { 2, 1000, 10 },
                                          { 0.01, 1.0, 0.05 },
                                          { 1, 80, 1 },
                                          { 0.01, 1.0, 0.05 }} );
            expd.setStepsSpinBoxConfigs({ { 2, 50, 1 },
                                          { 2, 50, 1 },
                                          { 2, 50, 1 },
                                          { 2, 50, 1 },
                                          { 2, 50, 1 },
                                          { 2, 50, 1 }} );
            break;
        }
    }

    //show the dialog modally
    int ret = expd.exec();
    if( ret != QDialog::Accepted )
        return;

    //perform the experiments
    {
        switch ( tabIndex ) {
        case 1: //SA+GD algorithm
            runExperimentsWithSAandGD( expd );
            break;
        case 2: //LSRS algorithm
            runExperimentsWithLSRS( expd );
            break;
        case 3: //PSO algorithm
            runExperimentsWithPSO( expd );
            break;
        case 4: //Genetic algorithm
            runExperimentsWithGenetic( expd );
            break;
        }
    }


//    //open a new file for output
//    QFile outputFile( QString("C:\\Users\\ur5m\\Desktop\\experiments.txt") );
//    outputFile.open( QFile::WriteOnly | QFile::Text );
//    QTextStream out(&outputFile);

//    int experimentNumber = 1;
//    for( double tInitial = 20; tInitial <= 20000; tInitial += 2000 )
//        for( double tFinal = 20; tFinal <= 10000 && tFinal < tInitial; tFinal += 1000 )
//            for( int nSteps = 100; nSteps <= 5000; nSteps += 500 )
//                for( double hopFactor = 0.01; hopFactor <= 1.000 ; hopFactor += 0.1 )
//                {
//                    std::vector< IJVariographicStructure2D > model =
//                                  m_autoVarFit.processWithSAandGD(
//                                                     8,
//                                                     4,
//                                                     131313,
//                                                     tInitial,
//                                                     tFinal,
//                                                     nSteps,
//                                                     hopFactor,
//                                                     20,
//                                                     0.000001,
//                                                     1.0,
//                                                     60,
//                                                     0.000001,
//                                                     false);
//                    out << "t0=" << tInitial
//                              << " t1=" << tFinal
//                              << " nSteps=" << nSteps
//                              << " hop=" << hopFactor
//                              << " F=" << m_autoVarFit.evaluateModel( model )
//                              << '\n';
//                    out.flush();
//                    std::cout << experimentNumber << std::endl;
//                    ++experimentNumber;
//                }

//    //closes the output file
//    outputFile.close();
}

void AutomaticVarFitDialog::onObjectiveFunctionChanged()
{
    if( ui->cmbObjectiveFunction->currentIndex() == 0 )
        m_autoVarFit.setObjectiveFunctionType( ObjectiveFunctionType::BASED_ON_FIM );
    else
        m_autoVarFit.setObjectiveFunctionType( ObjectiveFunctionType::BASED_ON_VARFIT );
}

void AutomaticVarFitDialog::onMethodTabChanged(int tabIndex)
{
    if( tabIndex >=1 && tabIndex <= 4)
        ui->btnRunExperiments->setEnabled( true );
    else
        ui->btnRunExperiments->setEnabled( false );
}

void AutomaticVarFitDialog::runExperimentsWithSAandGD(const AutomaticVarFitExperimentsDialog& expParDiag)
{
    switch ( expParDiag.getParameterIndex() ) {
    case 0: //vary seed
        runExperimentsWithSAandGD( expParDiag.getFrom(), expParDiag.getTo(), expParDiag.getNumberOfSteps(),
                                   ui->spinInitialTemperature->value(), ui->spinInitialTemperature->value(), 1,
                                   ui->spinFinalTemperature->value(), ui->spinFinalTemperature->value(), 1,
                                   ui->spinMaxHopFactor->value(), ui->spinMaxHopFactor->value(), 1);
        break;
    case 1: //vary initial temperature
        runExperimentsWithSAandGD( ui->spinSeed->value(), ui->spinSeed->value(), 1,
                                   expParDiag.getFrom(), expParDiag.getTo(), expParDiag.getNumberOfSteps(),
                                   ui->spinFinalTemperature->value(), ui->spinFinalTemperature->value(), 1,
                                   ui->spinMaxHopFactor->value(), ui->spinMaxHopFactor->value(), 1);
        break;
    case 2: //vary final temperature
        runExperimentsWithSAandGD( ui->spinSeed->value(), ui->spinSeed->value(), 1,
                                   ui->spinInitialTemperature->value(), ui->spinInitialTemperature->value(), 1,
                                   expParDiag.getFrom(), expParDiag.getTo(), expParDiag.getNumberOfSteps(),
                                   ui->spinMaxHopFactor->value(), ui->spinMaxHopFactor->value(), 1);
        break;
    case 3: //vary max. 'hop' factor
        runExperimentsWithSAandGD( ui->spinSeed->value(), ui->spinSeed->value(), 1,
                                   ui->spinInitialTemperature->value(), ui->spinInitialTemperature->value(), 1,
                                   ui->spinFinalTemperature->value(), ui->spinFinalTemperature->value(), 1,
                                   expParDiag.getFrom(), expParDiag.getTo(), expParDiag.getNumberOfSteps());
        break;
    }
}

void AutomaticVarFitDialog::runExperimentsWithSAandGD(
        int seedI,       int seedF,       int seedSteps,
        double iniTempI, double iniTempF, int iniTempSteps,
        double finTempI, double finTempF, int finTempSteps,
        double hopFactI, double hopFactF, int hopFactSteps
        )
{
    //-----------------set the experiment parameter ranges------------------
    int seedStep = ( seedF - seedI ) / seedSteps;
    if( seedStep <= 0 ) seedStep = 1000000; //makes sure the loop executes just once if initial == final

    double iniTempStep = ( iniTempF - iniTempI ) / iniTempSteps;
    if( iniTempStep <= 0.0 ) iniTempStep = 1000000.0; //makes sure the loop executes just once if initial == final

    double finTempStep = ( finTempF - finTempI ) / finTempSteps;
    if( finTempStep <= 0.0 ) finTempStep = 1000000.0; //makes sure the loop executes just once if initial == final

    double hopFactStep = ( hopFactF - hopFactI ) / hopFactSteps;
    if( hopFactStep <= 0.0 ) hopFactStep = 1000000.0; //makes sure the loop executes just once if initial == final

    //------------------populate the curves (runs the experiment)---------------------------------
    std::vector< std::pair< QString, std::vector< double > > > convergenceCurves;
    for( int seed = seedI; seed <= seedF; seed += seedStep )
        for( double tInitial = iniTempI; tInitial <= iniTempF; tInitial += iniTempStep )
            for( double tFinal = finTempI; tFinal <= finTempF && tFinal < tInitial; tFinal += finTempStep )
                for( double hopFactor = hopFactI; hopFactor <= hopFactF; hopFactor += hopFactStep ){
                    //Run the algorithm
                    std::vector< IJVariographicStructure2D > model =
                                  m_autoVarFit.processWithSAandGD(
                                                     ui->spinNumberOfThreads->value(),
                                                     ui->spinNumberOfVariogramStructures->value(),
                                                     seed,
                                                     tInitial,
                                                     tFinal,
                                                     ui->spinMaxStepsSA->value(),
                                                     hopFactor,
                                                     ui->spinMaxSteps->value(),
                                                     std::pow( 10.0, ui->spinLogEpsilon->value() ),
                                                     ui->spinInitialAlpha->value(),
                                                     ui->spinMaxStepsAlphaReduction->value(),
                                                     std::pow( 10.0, ui->spinConvergenceCriterion->value() ),
                                                     false);
                    //collect the convergence profile (evolution of the objective function
                    //value as the iteration progresses)
                    convergenceCurves.push_back( {
                                                     QString("seed=%1;Ti=%2;Tf=%3;hop=%4").arg(seed).arg(tInitial).arg(tFinal).arg(hopFactor),
                                                     m_autoVarFit.getObjectiveFunctionValuesOfLastRun()
                                                 } );
                }

    //----------------Set chart title and show the curves--------------------
    QStringList varyingWhat;
    if( seedSteps > 1 )
        varyingWhat += "seed";
    if( iniTempSteps > 1 )
        varyingWhat += "Ti";
    if( finTempSteps > 1 )
        varyingWhat += "Tf";
    if( hopFactSteps > 1 )
        varyingWhat += "hop";
    showConvergenceCurves( "SA/GD: varying " + Util::formatAsSingleLine( varyingWhat, ", " ), convergenceCurves );
}

void AutomaticVarFitDialog::runExperimentsWithLSRS( const AutomaticVarFitExperimentsDialog& expParDiag )
{
    switch ( expParDiag.getParameterIndex() ) {
    case 0: //vary seed
        runExperimentsWithLSRS( expParDiag.getFrom(), expParDiag.getTo(), expParDiag.getNumberOfSteps(),
                                   ui->spinNumberOfStartingPoints->value(), ui->spinNumberOfStartingPoints->value(), 1);
        break;
    case 1: //vary number of lines
        runExperimentsWithLSRS( ui->spinSeed->value(), ui->spinSeed->value(), 1,
                                   expParDiag.getFrom(), expParDiag.getTo(), expParDiag.getNumberOfSteps()
                              );
        break;
    }
}

void AutomaticVarFitDialog::runExperimentsWithLSRS(int seedI, int seedF, int seedSteps,
                                                   double nLinesI, double nLinesF, int nLinesSteps)
{
    //-----------------set the experiment parameter ranges------------------
    int seedStep = ( seedF - seedI ) / seedSteps;
    if( seedStep <= 0 ) seedStep = 1000000; //makes sure the loop executes just once if initial == final

    double nLinesStep = ( nLinesF - nLinesI ) / nLinesSteps;
    if( nLinesStep <= 0.0 ) nLinesStep = 1000000.0; //makes sure the loop executes just once if initial == final

    //------------------populate the curves (runs the experiment)---------------------------------
    std::vector< std::pair< QString, std::vector< double > > > convergenceCurves;
    for( int seed = seedI; seed <= seedF; seed += seedStep )
        for( double nLines = nLinesI; nLines <= nLinesF; nLines += nLinesStep ){
            //Run the algorithm
            std::vector< IJVariographicStructure2D > model =
                          m_autoVarFit.processWithLSRS(
                                             ui->spinNumberOfThreads->value(),
                                             ui->spinNumberOfVariogramStructures->value(),
                                             seed,
                                             ui->spinMaxStepsLSRS->value(),
                                             std::pow( 10.0, ui->spinLogEpsilonLSRS->value() ),
                                             nLines,
                                             ui->spinNumberOfRestarts->value(),
                                             false);
            //collect the convergence profile (evolution of the objective function
            //value as the iteration progresses)
            convergenceCurves.push_back( {
                                             QString("seed=%1;nLines=%2").arg(seed).arg(nLines),
                                             m_autoVarFit.getObjectiveFunctionValuesOfLastRun()
                                         } );
        }

    //----------------Set chart title and show the curves--------------------
    QStringList varyingWhat;
    if( seedSteps > 1 )
        varyingWhat += "seed";
    if( nLinesSteps > 1 )
        varyingWhat += "nLines";
    showConvergenceCurves( "LSRS: varying " + Util::formatAsSingleLine( varyingWhat, ", " ), convergenceCurves );
}

void AutomaticVarFitDialog::runExperimentsWithPSO( const AutomaticVarFitExperimentsDialog& expParDiag )
{
    switch ( expParDiag.getParameterIndex() ) {
    case 0: //vary seed
        runExperimentsWithPSO( expParDiag.getFrom(), expParDiag.getTo(), expParDiag.getNumberOfSteps(),
                               ui->spinNumberOfParticles->value(), ui->spinNumberOfParticles->value(), 1,
                               ui->dblSpinInertiaWeight->value(), ui->dblSpinInertiaWeight->value(), 1,
                               ui->dblSpinAccelerationConstant1->value(), ui->dblSpinAccelerationConstant1->value(), 1,
                               ui->dblSpinAccelerationConstant2->value(), ui->dblSpinAccelerationConstant2->value(), 1 );
        break;
    case 1: //vary number of particles
        runExperimentsWithPSO( ui->spinSeed->value(), ui->spinSeed->value(), 1,
                               expParDiag.getFrom(), expParDiag.getTo(), expParDiag.getNumberOfSteps(),
                               ui->dblSpinInertiaWeight->value(), ui->dblSpinInertiaWeight->value(), 1,
                               ui->dblSpinAccelerationConstant1->value(), ui->dblSpinAccelerationConstant1->value(), 1,
                               ui->dblSpinAccelerationConstant2->value(), ui->dblSpinAccelerationConstant2->value(), 1 );
        break;
    case 2: //vary inertia
        runExperimentsWithPSO( ui->spinSeed->value(), ui->spinSeed->value(), 1,
                               ui->spinNumberOfParticles->value(), ui->spinNumberOfParticles->value(), 1,
                               expParDiag.getFrom(), expParDiag.getTo(), expParDiag.getNumberOfSteps(),
                               ui->dblSpinAccelerationConstant1->value(), ui->dblSpinAccelerationConstant1->value(), 1,
                               ui->dblSpinAccelerationConstant2->value(), ui->dblSpinAccelerationConstant2->value(), 1 );
        break;
    case 3: //vary acceleration factor 1
        runExperimentsWithPSO( ui->spinSeed->value(), ui->spinSeed->value(), 1,
                               ui->spinNumberOfParticles->value(), ui->spinNumberOfParticles->value(), 1,
                               ui->dblSpinInertiaWeight->value(), ui->dblSpinInertiaWeight->value(), 1,
                               expParDiag.getFrom(), expParDiag.getTo(), expParDiag.getNumberOfSteps(),
                               ui->dblSpinAccelerationConstant2->value(), ui->dblSpinAccelerationConstant2->value(), 1 );
        break;
    case 4: //vary acceleration factor 2
        runExperimentsWithPSO( ui->spinSeed->value(), ui->spinSeed->value(), 1,
                               ui->spinNumberOfParticles->value(), ui->spinNumberOfParticles->value(), 1,
                               ui->dblSpinInertiaWeight->value(), ui->dblSpinInertiaWeight->value(), 1,
                               ui->dblSpinAccelerationConstant1->value(), ui->dblSpinAccelerationConstant1->value(), 1,
                               expParDiag.getFrom(), expParDiag.getTo(), expParDiag.getNumberOfSteps() );
        break;
    }
}

void AutomaticVarFitDialog::runExperimentsWithPSO(int    seedI,          int seedF,             int seedSteps,
                                                  double nParticlesI,    double nParticlesF,    int nParticlesSteps,
                                                  double inertiaI,       double inertiaF,       int inertiaSteps,
                                                  double acceleration1I, double acceleration1F, int acceleration1Steps,
                                                  double acceleration2I, double acceleration2F, int acceleration2Steps)
{
    //-----------------set the experiment parameter ranges------------------
    int seedStep = ( seedF - seedI ) / seedSteps;
    if( seedStep <= 0 ) seedStep = 1000000; //makes sure the loop executes just once if initial == final

    double nParticlesStep = ( nParticlesF - nParticlesI ) / nParticlesSteps;
    if( nParticlesStep <= 0.0 ) nParticlesStep = 1000000.0; //makes sure the loop executes just once if initial == final

    double inertiaStep = ( inertiaF - inertiaI ) / inertiaSteps;
    if( inertiaStep <= 0.0 ) inertiaStep = 1000000.0; //makes sure the loop executes just once if initial == final

    double acceleration1Step = ( acceleration1F - acceleration1I ) / acceleration1Steps;
    if( acceleration1Step <= 0.0 ) acceleration1Step = 1000000.0; //makes sure the loop executes just once if initial == final

    double acceleration2Step = ( acceleration2F - acceleration2I ) / acceleration2Steps;
    if( acceleration2Step <= 0.0 ) acceleration2Step = 1000000.0; //makes sure the loop executes just once if initial == final

    //------------------populate the curves (runs the experiment)---------------------------------
    std::vector< std::pair< QString, std::vector< double > > > convergenceCurves;
    for( int seed = seedI; seed <= seedF; seed += seedStep )
        for( double nParticles = nParticlesI; nParticles <= nParticlesF; nParticles += nParticlesStep )
            for( double inertia = inertiaI; inertia <= inertiaF; inertia += inertiaStep )
                for( double acceleration1 = acceleration1I; acceleration1 <= acceleration1F; acceleration1 += acceleration1Step )
                    for( double acceleration2 = acceleration2I; acceleration2 <= acceleration2F; acceleration2 += acceleration2Step ){
                        //Run the algorithm
                        std::vector< IJVariographicStructure2D > model =
                                      m_autoVarFit.processWithPSO(
                                                         ui->spinNumberOfVariogramStructures->value(),
                                                         seed,
                                                         ui->spinMaxStepsPSO->value(),
                                                         nParticles,
                                                         inertia,
                                                         acceleration1,
                                                         acceleration2,
                                                         false);
                        //collect the convergence profile (evolution of the objective function
                        //value as the iteration progresses)
                        convergenceCurves.push_back( {
                                                         QString("seed=%1;nPart=%2;inert=%3;acc1=%4;acc2=%5").
                                                                  arg(seed).arg(nParticles).arg(inertia).arg(acceleration1).arg(acceleration2),
                                                         m_autoVarFit.getObjectiveFunctionValuesOfLastRun()
                                                     } );
                    }

    //----------------Set chart title and show the curves--------------------
    QStringList varyingWhat;
    if( seedSteps > 1 )
        varyingWhat += "seed";
    if( nParticlesSteps > 1 )
        varyingWhat += "nParticles";
    if( inertiaSteps > 1 )
        varyingWhat += "inertia";
    if( acceleration1Steps > 1 )
        varyingWhat += "acceleration factor 1";
    if( acceleration2Steps > 1 )
        varyingWhat += "acceleration factor 2";
    showConvergenceCurves( "PSO: varying " + Util::formatAsSingleLine( varyingWhat, ", " ), convergenceCurves );
}

void AutomaticVarFitDialog::runExperimentsWithGenetic( const AutomaticVarFitExperimentsDialog& expParDiag )
{
    switch ( expParDiag.getParameterIndex() ) {
    case 0: //vary seed
        runExperimentsWithGenetic( expParDiag.getFrom(), expParDiag.getTo(), expParDiag.getNumberOfSteps(),
                                   ui->spinPopulationSize->value(), ui->spinPopulationSize->value(), 1,
                                   ui->spinSelectionSize->value(), ui->spinSelectionSize->value(), 1,
                                   ui->dblSpinProbabilityOfCrossover->value(), ui->dblSpinProbabilityOfCrossover->value(), 1,
                                   ui->spinPointOfCrossover->value(), ui->spinPointOfCrossover->value(), 1,
                                   ui->dblSpinMutationRate->value(), ui->dblSpinMutationRate->value(), 1 );
        break;
    case 1: //vary population size
        runExperimentsWithGenetic( ui->spinSeed->value(), ui->spinSeed->value(), 1,
                                   expParDiag.getFrom(), expParDiag.getTo(), expParDiag.getNumberOfSteps(),
                                   ui->spinSelectionSize->value(), ui->spinSelectionSize->value(), 1,
                                   ui->dblSpinProbabilityOfCrossover->value(), ui->dblSpinProbabilityOfCrossover->value(), 1,
                                   ui->spinPointOfCrossover->value(), ui->spinPointOfCrossover->value(), 1,
                                   ui->dblSpinMutationRate->value(), ui->dblSpinMutationRate->value(), 1 );
        break;
    case 2: //vary selection size
        runExperimentsWithGenetic( ui->spinSeed->value(), ui->spinSeed->value(), 1,
                                   ui->spinPopulationSize->value(), ui->spinPopulationSize->value(), 1,
                                   expParDiag.getFrom(), expParDiag.getTo(), expParDiag.getNumberOfSteps(),
                                   ui->dblSpinProbabilityOfCrossover->value(), ui->dblSpinProbabilityOfCrossover->value(), 1,
                                   ui->spinPointOfCrossover->value(), ui->spinPointOfCrossover->value(), 1,
                                   ui->dblSpinMutationRate->value(), ui->dblSpinMutationRate->value(), 1 );
        break;
    case 3: //vary cross over probability
        runExperimentsWithGenetic( ui->spinSeed->value(), ui->spinSeed->value(), 1,
                                   ui->spinPopulationSize->value(), ui->spinPopulationSize->value(), 1,
                                   ui->spinSelectionSize->value(), ui->spinSelectionSize->value(), 1,
                                   expParDiag.getFrom(), expParDiag.getTo(), expParDiag.getNumberOfSteps(),
                                   ui->spinPointOfCrossover->value(), ui->spinPointOfCrossover->value(), 1,
                                   ui->dblSpinMutationRate->value(), ui->dblSpinMutationRate->value(), 1 );
        break;
    case 4: //vary point of cross over
        runExperimentsWithGenetic( ui->spinSeed->value(), ui->spinSeed->value(), 1,
                                   ui->spinPopulationSize->value(), ui->spinPopulationSize->value(), 1,
                                   ui->spinSelectionSize->value(), ui->spinSelectionSize->value(), 1,
                                   ui->dblSpinProbabilityOfCrossover->value(), ui->dblSpinProbabilityOfCrossover->value(), 1,
                                   expParDiag.getFrom(), expParDiag.getTo(), expParDiag.getNumberOfSteps(),
                                   ui->dblSpinMutationRate->value(), ui->dblSpinMutationRate->value(), 1 );
        break;
    case 5: //vary mutation rate
        runExperimentsWithGenetic( ui->spinSeed->value(), ui->spinSeed->value(), 1,
                                   ui->spinPopulationSize->value(), ui->spinPopulationSize->value(), 1,
                                   ui->spinSelectionSize->value(), ui->spinSelectionSize->value(), 1,
                                   ui->dblSpinProbabilityOfCrossover->value(), ui->dblSpinProbabilityOfCrossover->value(), 1,
                                   ui->spinPointOfCrossover->value(), ui->spinPointOfCrossover->value(), 1,
                                   expParDiag.getFrom(), expParDiag.getTo(), expParDiag.getNumberOfSteps() );
        break;
    }
}

void AutomaticVarFitDialog::runExperimentsWithGenetic(int seedI,            int seedF,            int seedSteps,
                                                      double popSizeI,      double popSizeF,      int popSizeSteps,
                                                      double selSizeI,      double selSizeF,      int selSizeSteps,
                                                      double xOverProbI,    double xOverProbF,    int xOverProbSteps,
                                                      double pointOfXOverI, double pointOfXOverF, int pointOfXOverSteps,
                                                      double mutRateI,      double mutRateF,      int mutRateSteps)
{
    //-----------------set the experiment parameter ranges------------------
    int seedStep = ( seedF - seedI ) / seedSteps;
    if( seedStep <= 0 ) seedStep = 1000000; //makes sure the loop executes just once if initial == final

    double popSizeStep = ( popSizeF - popSizeI ) / popSizeSteps;
    if( popSizeStep <= 0.0 ) popSizeStep = 1000000.0; //makes sure the loop executes just once if initial == final

    double selSizeStep = ( selSizeF - selSizeI ) / selSizeSteps;
    if( selSizeStep <= 0.0 ) selSizeStep = 1000000.0; //makes sure the loop executes just once if initial == final

    double xOverProbStep = ( xOverProbF - xOverProbI ) / xOverProbSteps;
    if( xOverProbStep <= 0.0 ) xOverProbStep = 1000000.0; //makes sure the loop executes just once if initial == final

    double pointOfXOverStep = ( pointOfXOverF - pointOfXOverI ) / pointOfXOverSteps;
    if( pointOfXOverStep <= 0.0 ) pointOfXOverStep = 1000000.0; //makes sure the loop executes just once if initial == final

    double mutRateStep = ( mutRateF - mutRateI ) / mutRateSteps;
    if( mutRateStep <= 0.0 ) mutRateStep = 1000000.0; //makes sure the loop executes just once if initial == final

    //------------------populate the curves (runs the experiment)---------------------------------
    std::vector< std::pair< QString, std::vector< double > > > convergenceCurves;
    for( int seed = seedI; seed <= seedF; seed += seedStep )
        for( double popSize = popSizeI; popSize <= popSizeF; popSize += popSizeStep )
            for( double selSize = selSizeI; selSize <= selSizeF; selSize += selSizeStep )
                for( double xOverProb = xOverProbI; xOverProb <= xOverProbF; xOverProb += xOverProbStep )
                    for( double pointOfXOver = pointOfXOverI; pointOfXOver <= pointOfXOverF; pointOfXOver += pointOfXOverStep )
                        for( double mutRate = mutRateI; mutRate <= mutRateF; mutRate += mutRateStep ){

                            uint iPopSize = static_cast<uint>(popSize);
                            if( iPopSize % 2 ) //if population size is odd
                                iPopSize++; //make it even

                            //Run the algorithm
                            std::vector< IJVariographicStructure2D > model =
                                          m_autoVarFit.processWithGenetic(
                                                             ui->spinNumberOfThreads->value(),
                                                             ui->spinNumberOfVariogramStructures->value(),
                                                             seed,
                                                             ui->spinNumberOfGenerations->value(),
                                                             iPopSize,
                                                             selSize,
                                                             xOverProb,
                                                             pointOfXOver,
                                                             mutRate,
                                                             false);
                            //collect the convergence profile (evolution of the objective function
                            //value as the iteration progresses)
                            convergenceCurves.push_back( {
                                                             QString("seed=%1;popSize=%2;selSize=%3;xOverP=%4;pointXOver=%5;mutRate=%6").
                                                                      arg(seed).arg(iPopSize).arg((int)selSize).arg(xOverProb).arg((int)pointOfXOver).arg(mutRate),
                                                             m_autoVarFit.getObjectiveFunctionValuesOfLastRun()
                                                         } );
                    }

    //----------------Set chart title and show the curves--------------------
    QStringList varyingWhat;
    if( seedSteps > 1 )
        varyingWhat += "seed";
    if( popSizeSteps > 1 )
        varyingWhat += "population size";
    if( selSizeSteps > 1 )
        varyingWhat += "selection size";
    if( xOverProbSteps > 1 )
        varyingWhat += "probability of cross over";
    if( pointOfXOverSteps > 1 )
        varyingWhat += "point of cross over";
    if( mutRateSteps > 1 )
        varyingWhat += "mutation rate";
    showConvergenceCurves( "GA: varying " + Util::formatAsSingleLine( varyingWhat, ", " ), convergenceCurves );
}

void AutomaticVarFitDialog::showConvergenceCurves(
        QString chartTitle,
        const std::vector<std::pair<QString, std::vector<double> > > &curves) const
{

    //load the multiple x,y data series for the chart
    std::vector< QtCharts::QLineSeries* > chartSeriesVector;
    double max = std::numeric_limits<double>::lowest();
    uint maxColorCode = Util::getMaxGSLibColorCode();
    uint iterationNumber = 0;
    for( const std::pair< QString, std::vector<double> >& curve : curves ){
        QtCharts::QLineSeries *chartSeries = new QtCharts::QLineSeries();
        //set the curve's legend caption
        chartSeries->setName( curve.first );
        //set the curve's color (cycle through a finite set of colors)
        QColor color = Util::getGSLibColor( ( iterationNumber % maxColorCode ) + 1 );
        if( color == Qt::white )
            color = Qt::black;
        chartSeries->setColor( color );
        //set the curve's values
        for(uint i = 0; i < curve.second.size(); ++i){
            chartSeries->append( i+1, curve.second[i] );
            if( curve.second[i] > max )
                max = curve.second[i];
        }
        chartSeriesVector.push_back( chartSeries );
        iterationNumber++;
    }

    //create a new chart object
    QtCharts::QChart *objFuncValuesChart = new QtCharts::QChart();
    {
        objFuncValuesChart->setTitle("<H2>" + chartTitle + "</H2>");

        QtCharts::QValueAxis* axisX = new QtCharts::QValueAxis();
        axisX->setLabelFormat("%i");
        axisX->setTitleText("iterations");

        QtCharts::QValueAxis *axisY = new QtCharts::QValueAxis();
        axisY->setLabelFormat("%3.2f");
        axisY->setRange( 0.0, max );
        axisY->setTitleText("obj. function value");

        for( QtCharts::QLineSeries* chartSeries : chartSeriesVector ){
            objFuncValuesChart->addSeries( chartSeries );
            objFuncValuesChart->axisX( chartSeries );
            objFuncValuesChart->setAxisX(axisX, chartSeries);
            objFuncValuesChart->setAxisY(axisY, chartSeries);
        }

        //objFuncValuesChart->legend()->hide();
        objFuncValuesChart->legend()->setAlignment( Qt::AlignRight );
    }

    //create the chart dialog
    EmptyDialog* ed = new EmptyDialog( Application::instance()->getMainWindow() );
    QtCharts::QChartView* chartView = new QtCharts::QChartView( objFuncValuesChart );
    ed->addWidget( chartView );
    ed->setWindowTitle( chartTitle );
    ed->exec();
}
