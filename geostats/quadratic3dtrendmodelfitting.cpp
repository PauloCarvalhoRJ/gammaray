#include "quadratic3dtrendmodelfitting.h"

#include "util.h"
#include "mainwindow.h"
#include "dialogs/emptydialog.h"
#include "domain/application.h"
#include "domain/attribute.h"
#include "domain/datafile.h"
#include "geometry/boundingbox.h"

#include <QApplication>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QMessageBox>
#include <QProgressDialog>
#include <QValueAxis>
#include <thread>

std::vector< double > Quadratic3DTrendModelFitting::s_objectiveFunctionValues;

////////////////////////////////////////CLASS FOR THE GENETIC ALGORITHM//////////////////////////////////////////

class Individual{
public:
    //constructors
    Individual() :
        m_parameters(),
        m_fValue( std::numeric_limits<double>::max() )
    {}
    Individual( const Quad3DTrendModelFittingAuxDefs::Parameters& pparameters ) :
        m_parameters( pparameters ),
        m_fValue( std::numeric_limits<double>::max() )
    {}
    Individual( const Individual& otherIndividual ) :
        m_parameters( otherIndividual.m_parameters ),
        m_fValue( otherIndividual.m_fValue )
    {}

    //genetic operators
    std::pair<Individual, Individual> crossOver( const Individual& otherIndividual,
                                                 int pointOfCrossOver ) const {
        Individual child1, child2;
        for( int iParameter = 0; iParameter < Quad3DTrendModelFittingAuxDefs::N_PARAMS; ++iParameter ){
            if( iParameter < pointOfCrossOver ){
                child1.m_parameters[iParameter] = m_parameters[iParameter];
                child2.m_parameters[iParameter] = otherIndividual.m_parameters[iParameter];
            } else {
                child1.m_parameters[iParameter] = otherIndividual.m_parameters[iParameter];
                child2.m_parameters[iParameter] = m_parameters[iParameter];
            }
        }
        return { child1, child2 };
    }
    void mutate( double mutationRate,
                 const Quad3DTrendModelFittingAuxDefs::Parameters& lowBoundaries,
                 const Quad3DTrendModelFittingAuxDefs::Parameters& highBoundaries ){
        //compute the mutation probability for a single gene (parameter)
        double probOfMutation = 1.0 / Quad3DTrendModelFittingAuxDefs::N_PARAMS * mutationRate;
        //traverse all genes (parameters)
        for( int iPar = 0; iPar < Quad3DTrendModelFittingAuxDefs::N_PARAMS; ++iPar ){
            //draw a value between 0.0 and 1.0 from an uniform distribution
            double p = std::rand() / (double)RAND_MAX;
            //if a mutation is due...
            if( p < probOfMutation ) {
                //perform mutation by randomly sorting a value within the domain.
                double LO = lowBoundaries[iPar];
                double HI = highBoundaries[iPar];
                m_parameters[iPar] = LO + std::rand() / (RAND_MAX/(HI-LO));
            }
        }
    }

    //attribution operator
    Individual& operator=( const Individual& otherIndividual ){
        m_parameters = otherIndividual.m_parameters;
        m_fValue = otherIndividual.m_fValue;
        return *this;
    }

    //comparison operator
    bool operator<( const Individual& otherIndividual ) const {
        return m_fValue < otherIndividual.m_fValue;
    }

    //member variables
    Quad3DTrendModelFittingAuxDefs::Parameters m_parameters; //the genes (model parameters) of this individual
    double m_fValue; //objective function value corresponding to this individual
};
typedef Individual Solution; //make a synonym just for code readbility
/////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * The code for multithreaded evaluation of the objective function for a range of individuals (a set of variogam parameters)
 * in the Genetic Algorithm.
 * @param quad3DTrendModelFitRef Reference to the Quadratic3DTrendModelFitting object so its objective function can be called.
 * @param iIndividual_initial First individual index to process.
 * @param iIndividual_final Last individual index to process.  If final_i==initial_i, this thread processes only one individual.
 * OUTPUT PARAMETER:
 * @param population The set of individuals to receive the evaluation of the objective function.
 */
void taskEvaluateObjetiveInRangeOfIndividualsForGenetic(
        const Quadratic3DTrendModelFitting& quad3DTrendModelFitRef,
        int iIndividual_initial,
        int iIndividual_final,
        std::vector< Individual >& population  //-->output parameter
        ) {
    for( int iInd = iIndividual_initial; iInd <= iIndividual_final; ++iInd ){
        Individual& ind = population[iInd];
        ind.m_fValue = quad3DTrendModelFitRef.objectiveFunction( ind.m_parameters );
    }
}


Quadratic3DTrendModelFitting::Quadratic3DTrendModelFitting( DataFile* dataFile, Attribute* attribute ) :
    m_attribute(attribute), m_dataFile(dataFile)
{}

double Quadratic3DTrendModelFitting::objectiveFunction( const Quad3DTrendModelFittingAuxDefs::Parameters& parameters ) const
{
    //the sum to compute the error mean to be returned
    double sum = 0.0;

    //get the no-data value in numeric form to improve performance
    double NDV = m_dataFile->getNoDataValueAsDouble();
    bool hasNDV = m_dataFile->hasNoDataValue();

    //if data is 2D, then the z-bearing terms are invariant.
    constexpr int IS_3D = 1;
    constexpr int IS_2D = 0;
    int is3D = m_dataFile->isTridimensional() ? IS_3D : IS_2D;

    uint16_t indexOfDependentVariable = m_attribute->getAttributeGEOEASgivenIndex()-1;

    //for each observation
    uint64_t count = 0;
    for( int iRow = 0; iRow < m_dataFile->getDataLineCount(); iRow++ ){
        //get the observed value
        double observedValue = m_dataFile->dataConst( iRow, indexOfDependentVariable );
        //if observed value is valid (not no-data-value)
        if( ! hasNDV || ! Util::almostEqual2sComplement( observedValue, NDV, 1 ) ){
            // get the xyz location of the current sample.
            double x, y, z;
            m_dataFile->getDataSpatialLocation( iRow, x, y, z );
            // use the current trend model (defined by its parameters)
            // to predict the value at the data location
            double predictedValue;
            switch (is3D) { //if is slower than a switch
                case IS_3D:
                    predictedValue = parameters.a * x * x +
                    parameters.b * x * y +
                    parameters.c * x * z +
                    parameters.d * y * y +
                    parameters.e * y * z +
                    parameters.f * z * z +
                    parameters.g * x +
                    parameters.h * y +
                    parameters.i * z ;
                    break;
                case IS_2D: //z-bearing terms are invariant in 2D datasets
                    predictedValue = parameters.a * x * x +
                    parameters.b * x * y +
                    parameters.d * y * y +
                    parameters.g * x +
                    parameters.h * y;
            }
            //accumulate the observed - predicted differences (prediction errors)
            double error = observedValue - predictedValue;
            sum += std::abs(error);
            count++;
        }
    }
    return sum / count;
}

void Quadratic3DTrendModelFitting::initDomain( Quad3DTrendModelFittingAuxDefs::ParametersDomain& domain,
                                               double searchWindowSize ) const
{
    for( int i = 0; i < Quad3DTrendModelFittingAuxDefs::N_PARAMS; ++i){
        //define the domain boundaries
        domain.min[i] = -searchWindowSize;
        domain.max[i] =  searchWindowSize;
    }
}

Quad3DTrendModelFittingAuxDefs::Parameters Quadratic3DTrendModelFitting::processWithGenetic(
        uint16_t nThreads, uint32_t seed, uint16_t maxNumberOfGenerations,
        uint16_t nPopulationSize, uint16_t nSelectionSize, double probabilityOfCrossOver,
        uint8_t pointOfCrossover, double mutationRate, double searchWindowSize, double windowWindowShiftThreshold) const
{

    //clear the collected objective function values.
    s_objectiveFunctionValues.clear();

    //Intialize the random number generator with the same seed
    std::srand (seed);

    //sanity checks
    if( nSelectionSize >= nPopulationSize ){
        QMessageBox::critical( Application::instance()->getMainWindow(),
                               "Error",
                               "Quadratic3DTrendModelFitting::processWithGenetic(): Selection pool size must be less than population size.");
        return Quad3DTrendModelFittingAuxDefs::Parameters();
    }
    if( nPopulationSize % 2 + nSelectionSize % 2 ){
        QMessageBox::critical( Application::instance()->getMainWindow(),
                               "Error",
                               "Quadratic3DTrendModelFitting::processWithGenetic(): Sizes must be even numbers.");
        return Quad3DTrendModelFittingAuxDefs::Parameters();
    }
    if( pointOfCrossover >= Quad3DTrendModelFittingAuxDefs::N_PARAMS ){
        QMessageBox::critical( Application::instance()->getMainWindow(),
                               "Error",
                               "Quadratic3DTrendModelFitting::processWithGenetic(): Point of crossover must be less than the number of parameters.");
        return Quad3DTrendModelFittingAuxDefs::Parameters();
    }

    // Fetch data from the data source.
    m_dataFile->loadData();

    //Initialize the optimization domain (boundary conditions)
    Quad3DTrendModelFittingAuxDefs::ParametersDomain domain;
    Quad3DTrendModelFittingAuxDefs::Parameters domainCenter = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0}; //init domain center at origin of the 9-D space
    initDomain( domain, searchWindowSize );

    //initialize the best fit paramaters in the domain center
    Quad3DTrendModelFittingAuxDefs::Parameters gbest_pw = domainCenter;

    //if data is 2D, then the z-bearing terms are invariant.
    constexpr int IS_3D = 1;
    constexpr int IS_2D = 0;
    int dimensionality = m_dataFile->isTridimensional() ? IS_3D : IS_2D;

    //loop for the domain window shift if a solution is near the current domain boundaries
    //executes at least once
    while(true){

        //=========================================THE GENETIC ALGORITHM==================================================

        //distribute as evenly as possible (load balance) the starting
        //points (by their indexes) amongst the threads.
        std::vector< std::pair< int, int > > individualsIndexesRanges =
                Util::generateSubRanges( 0, nPopulationSize-1, nThreads );

        //sanity check
        assert( individualsIndexesRanges.size() == nThreads && "Quadratic3DTrendModelFitting::processWithGenetic(): "
                                                               "number of threads different from individual index ranges. "
                                                               " This is likely a bug in Util::generateSubRanges() function." );

        QProgressDialog progressDialog;
        progressDialog.setRange(0, maxNumberOfGenerations);
        progressDialog.setValue( 0 );
        progressDialog.show();
        progressDialog.setLabelText("Genetic Algorithm in progress...");

        //the main genetic algorithm algorithm loop
        std::vector< Individual > population;
        for( int iGen = 0; iGen < maxNumberOfGenerations; ++iGen ){

            //the current best solution found so far is kept as part of the population
            if( ! gbest_pw.isTrivial() ) {
                Individual currentBest( gbest_pw );
                population.push_back( currentBest );
            }

            //complete the population with randomly generated individuals.
            while( population.size() < nPopulationSize ){
                //create a set of genes (parameters)
                Quad3DTrendModelFittingAuxDefs::Parameters pw;
                //randomize the individual's position in the domain.
                for( int i = 0; i < Quad3DTrendModelFittingAuxDefs::N_PARAMS; ++i ){
                    double LO = domain.min[i];
                    double HI = domain.max[i];
                    pw[i] = LO + std::rand() / (RAND_MAX/(HI-LO));
                }
                //zero-out z-bearing parameters if data set is 2D
                if( dimensionality == IS_2D )
                    pw.zeroOutZBearingTerms();
                //create an individual with the random genes
                Individual ind( pw );
                //add the new individual to the population
                population.push_back( ind );
            }

            //create and start the threads.  Each thread evaluates the objective function for a series of individuals.
            std::thread threads[nThreads];
            unsigned int iThread = 0;
            for( const std::pair< int, int >& individualsIndexesRange : individualsIndexesRanges ) {
                threads[iThread] = std::thread( taskEvaluateObjetiveInRangeOfIndividualsForGenetic,
                                                std::cref(*this),
                                                individualsIndexesRange.first,
                                                individualsIndexesRange.second,
                                                std::ref( population ) //--> OUTPUT PARAMETER
                                                );
                ++iThread;
            } //for each thread (ranges of starting points)

            //wait for the threads to finish.
            for( unsigned int iThread = 0; iThread < nThreads; ++iThread)
                threads[iThread].join();

            //sort the population in ascending order (lower value == better fitness)
            std::sort( population.begin(), population.end() );

            //collect the iteration's best objective function value
            s_objectiveFunctionValues.push_back( population[0].m_fValue );

            //clip the population (the excessive worst fit individuals die)
            while( population.size() > nPopulationSize )
                population.pop_back();

            //perform selection by binary tournament
            std::vector< Individual > selection;
            for( uint iSel = 0; iSel < nSelectionSize; ++iSel ){
                //perform binary tournament
                std::vector< Individual > tournament;
                {
                    //draw two different individuals at random from the population for the tournament.
                    int tournCandidate1 = std::rand() / (double)RAND_MAX * ( population.size() - 1 );
                    int tournCandidate2 = tournCandidate1;
                    while( tournCandidate2 == tournCandidate1 )
                        tournCandidate2 = std::rand() / (double)RAND_MAX * ( population.size() - 1 );
                    //add the participants in the tournament
                    tournament.push_back( population[tournCandidate1] );
                    tournament.push_back( population[tournCandidate2] );
                    //sort the binary tournament
                    std::sort( tournament.begin(), tournament.end());
                }
                //add the best of tournament to the selection pool
                selection.push_back( tournament.front() );
            }

            //perform crossover and mutation on the selected individuals
            std::vector< Individual > nextGen;
            while( ! selection.empty() ){
                //draw two different selected individuals at random for crossover.
                int parentIndex1 = std::rand() / (double)RAND_MAX * ( selection.size() - 1 );
                int parentIndex2 = parentIndex1;
                while( parentIndex2 == parentIndex1 )
                    parentIndex2 = std::rand() / (double)RAND_MAX * ( selection.size() - 1 );
                Individual parent1 = selection[ parentIndex1 ];
                Individual parent2 = selection[ parentIndex2 ];
                selection.erase( selection.begin() + parentIndex1 );
                selection.erase( selection.begin() + parentIndex2 );
                //draw a value between 0.0 and 1.0 from an uniform distribution
                double p = std::rand() / (double)RAND_MAX;
                //if crossover is due...
                if( p < probabilityOfCrossOver ){
                    //crossover
                    std::pair< Individual, Individual> offspring = parent1.crossOver( parent2, pointOfCrossover );
                    Individual child1 = offspring.first;
                    Individual child2 = offspring.second;
                    //mutate all
                    child1.mutate( mutationRate, domain.min, domain.max );
                    child2.mutate( mutationRate, domain.min, domain.max );
                    parent1.mutate( mutationRate, domain.min, domain.max );
                    parent2.mutate( mutationRate, domain.min, domain.max );
                    if( dimensionality == IS_2D ){ //zero-out z-bearing parameters (genes)
                        child1.m_parameters.zeroOutZBearingTerms();
                        child2.m_parameters.zeroOutZBearingTerms();
                        parent1.m_parameters.zeroOutZBearingTerms();
                        parent2.m_parameters.zeroOutZBearingTerms();
                    }
                    //add them to the next generation pool
                    nextGen.push_back( child1 );
                    nextGen.push_back( child2 );
                    nextGen.push_back( parent1 );
                    nextGen.push_back( parent2 );
                } else { //no crossover took place
                    //simply mutate and insert the parents into the next generation pool
                    parent1.mutate( mutationRate, domain.min, domain.max );
                    parent2.mutate( mutationRate, domain.min, domain.max );
                    if( dimensionality == IS_2D ){  //zero-out z-bearing parameters (genes)
                        parent1.m_parameters.zeroOutZBearingTerms();
                        parent2.m_parameters.zeroOutZBearingTerms();
                    }
                    nextGen.push_back( parent1 );
                    nextGen.push_back( parent2 );
                }
            }

            //make the next generation
            population = nextGen;

            //update progress bar
            progressDialog.setValue( iGen );
            QApplication::processEvents(); // let Qt update the UI

        } //main algorithm loop

        //=====================================GET RESULTS========================================
        progressDialog.hide();

        //evaluate the individuals of final population
        for( uint iInd = 0; iInd < population.size(); ++iInd ){
            Individual& ind = population[iInd];
            ind.m_fValue = objectiveFunction( ind.m_parameters );
        }

        //sort the population in ascending order (lower value == better fitness)
        std::sort( population.begin(), population.end() );

        //get the parameters of the best individual (set of parameters)
        gbest_pw = population[0].m_parameters;

        //if the solution is too close to the current domain boundaries, then
        //it is possible that the actual solution lies outside, then the program
        //shifts the domain so the current solution lies in its center and rerun
        //the genetic algorithm within the new search domain
        if( domain.isNearBoundary( gbest_pw, windowWindowShiftThreshold ) )
            domain.centerAt( gbest_pw, searchWindowSize );
        else
            break; //terminates the search if the solution is not near the domain boundaries

    } //loop for domain window shift if a solution is near the current domain boundaries

    // Display the results in a window.
    //displayResults( variogramStructures, inputFFTimagPhase, inputVarmap, false );
    showObjectiveFunctionEvolution();

    //return the fitted model
    return gbest_pw;
}

void Quadratic3DTrendModelFitting::showObjectiveFunctionEvolution() const
{
    //load the x,y data for the chart
    QtCharts::QLineSeries *chartSeries = new QtCharts::QLineSeries();
    double max = std::numeric_limits<double>::lowest();
    for(uint i = 0; i < s_objectiveFunctionValues.size(); ++i){
        chartSeries->append( i+1, s_objectiveFunctionValues[i] );
        if( s_objectiveFunctionValues[i] > max )
            max = s_objectiveFunctionValues[i];
    }

    //create a new chart object
    QtCharts::QChart *objFuncValuesChart = new QtCharts::QChart();
    {
        objFuncValuesChart->addSeries( chartSeries );
        objFuncValuesChart->axisX( chartSeries );

        QtCharts::QValueAxis* axisX = new QtCharts::QValueAxis();
        axisX->setLabelFormat("%i");
        objFuncValuesChart->setAxisX(axisX, chartSeries);

        QtCharts::QValueAxis *axisY = new QtCharts::QValueAxis();
        axisY->setLabelFormat("%3.2f");
        axisY->setRange( 0.0, max );
        objFuncValuesChart->setAxisY(axisY, chartSeries);

        objFuncValuesChart->legend()->hide();
    }

    //create the chart dialog
    EmptyDialog* ed = new EmptyDialog( Application::instance()->getMainWindow() );
    QtCharts::QChartView* chartView = new QtCharts::QChartView( objFuncValuesChart );
    ed->addWidget( chartView );
    ed->setWindowTitle( "Objective function with iterations." );
    ed->show();
}
