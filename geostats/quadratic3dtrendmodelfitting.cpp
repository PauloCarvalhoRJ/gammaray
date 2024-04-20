#include "quadratic3dtrendmodelfitting.h"

#include "util.h"
#include "mainwindow.h"
#include "domain/application.h"
#include "domain/attribute.h"
#include "domain/datafile.h"

#include <QApplication>
#include <QMessageBox>
#include <QProgressDialog>
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
    Individual( const Quadratic3DTrendModelFitting::Parameters& pparameters ) :
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
        for( int iParameter = 0; iParameter < Quadratic3DTrendModelFitting::N_PARAMS; ++iParameter ){
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
                 const Quadratic3DTrendModelFitting::Parameters& lowBoundaries,
                 const Quadratic3DTrendModelFitting::Parameters& highBoundaries ){
        //compute the mutation probability for a single gene (parameter)
        double probOfMutation = 1.0 / Quadratic3DTrendModelFitting::N_PARAMS * mutationRate;
        //traverse all genes (parameters)
        for( int iPar = 0; iPar < Quadratic3DTrendModelFitting::N_PARAMS; ++iPar ){
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
    Quadratic3DTrendModelFitting::Parameters m_parameters; //the genes (model parameters) of this individual
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

double Quadratic3DTrendModelFitting::objectiveFunction( const Quadratic3DTrendModelFitting::Parameters& parameters ) const
{
    double sum = 0.0;
    //get the no-data value in numeric form to improve performance
    double NDV = m_dataFile->getNoDataValueAsDouble();

    uint16_t indexOfDependentVariable = m_attribute->getAttributeGEOEASgivenIndex()-1;

    //for each observation
    for( int iRow = 0; iRow < m_dataFile->getDataLineCount(); iRow++ ){
        //get the observed value
        double observedValue = m_dataFile->dataConst( iRow, indexOfDependentVariable );
        //if observed value is valid (not no-data-value)
        if( ! Util::almostEqual2sComplement( observedValue, NDV, 1 ) ){
            // get the xyz location of the current sample.
            double x, y, z;
            m_dataFile->getDataSpatialLocation( iRow, x, y, z );
            // use the current trend model (defined by its parameters)
            // to predict the value at the data location
            double predictedValue =
                    parameters.a * x * x +
                    parameters.b * x * y +
                    parameters.c * x * z +
                    parameters.d * y * y +
                    parameters.e * y * z +
                    parameters.f * z * z +
                    parameters.g * x +
                    parameters.h * y +
                    parameters.i * z ;
            //accumulate the squares of the observed - predicted differences (prediction errors)
            double error = observedValue - predictedValue;
            sum += error*error;
        }
    }
    return sum;
}

void Quadratic3DTrendModelFitting::initDomainAndParameters( Quadratic3DTrendModelFitting::ParametersDomain& domain,
                                                            Quadratic3DTrendModelFitting::Parameters& parameters ) const
{
    for( int i = 0; i < Quadratic3DTrendModelFitting::N_PARAMS; ++i){
        //define the domain boundaries
        domain.min[i] = -1000.0;
        domain.max[i] =  1000.0;
        //and init the parameters near the center of the domain
        parameters[i] = ( domain.max[i] + domain.min[i] ) / 2;
    }
}

Quadratic3DTrendModelFitting::Parameters Quadratic3DTrendModelFitting::processWithGenetic(
        uint16_t nThreads, uint32_t seed, uint16_t maxNumberOfGenerations,
        uint16_t nPopulationSize, uint16_t nSelectionSize, double probabilityOfCrossOver,
        uint8_t pointOfCrossover, double mutationRate) const
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
        return Quadratic3DTrendModelFitting::Parameters();
    }
    if( nPopulationSize % 2 + nSelectionSize % 2 ){
        QMessageBox::critical( Application::instance()->getMainWindow(),
                               "Error",
                               "Quadratic3DTrendModelFitting::processWithGenetic(): Sizes must be even numbers.");
        return Quadratic3DTrendModelFitting::Parameters();
    }
    if( pointOfCrossover >= N_PARAMS ){
        QMessageBox::critical( Application::instance()->getMainWindow(),
                               "Error",
                               "Quadratic3DTrendModelFitting::processWithGenetic(): Point of crossover must be less than the number of parameters.");
        return Quadratic3DTrendModelFitting::Parameters();
    }

    // Fetch data from the data source.
    m_dataFile->loadData();

    //Initialize the optimization domain (boundary conditions) and
    //the sets of variogram paramaters (both linear and structured)
    ParametersDomain domain;
    Parameters parameters;
    initDomainAndParameters( domain,
                             parameters );

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

    //the main algorithm loop
    std::vector< Individual > population;
    for( int iGen = 0; iGen < maxNumberOfGenerations; ++iGen ){

        //Init or refill the population with randomly generated individuals.
        while( population.size() < nPopulationSize ){
            //create a set of genes (parameters)
            Parameters pw;
            //randomize the individual's position in the domain.
            for( int i = 0; i < N_PARAMS; ++i ){
                double LO = domain.min[i];
                double HI = domain.max[i];
                pw[i] = LO + std::rand() / (RAND_MAX/(HI-LO));
            }
            //create and individual with the random genes
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
                //add them to the next generation pool
                nextGen.push_back( child1 );
                nextGen.push_back( child2 );
                nextGen.push_back( parent1 );
                nextGen.push_back( parent2 );
            } else { //no crossover took place
                //simply mutate and insert the parents into the next generation pool
                parent1.mutate( mutationRate, domain.min, domain.max );
                parent2.mutate( mutationRate, domain.min, domain.max );
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
    Parameters gbest_pw = population[0].m_parameters;

    // Display the results in a window.
//    if( openResultsDialog ){
//        displayResults( variogramStructures, inputFFTimagPhase, inputVarmap, false );
//        showObjectiveFunctionEvolution();
//    }

    //return the fitted model
    return gbest_pw;
}
