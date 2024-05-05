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

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit_nlinear.h>

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

//locally defined data structure to store observation data in a form compatible with GSL's
//non-linear least squares solver.
struct data {
    size_t count;
    double* x;
    double* y;
    double* z;
    double* value;
};

//locally defined C-style function for use with GSL's non-linear least squares solver.
//this function returns the resuiduals (predicted - observed) in residuals_at_data_locations.
int cost_f (const gsl_vector* model_parameters_x,
            void* data,
            gsl_vector* residuals_at_data_locations) {

    // get data information
    size_t data_count =  static_cast<struct data *>(data)->count;
    double *x =          static_cast<struct data *>(data)->x;
    double *y =          static_cast<struct data *>(data)->y;
    double *z =          static_cast<struct data *>(data)->z;
    double *data_value = static_cast<struct data *>(data)->value;

    // get trend model parameters
    double p0 = gsl_vector_get (model_parameters_x, 0);
    double p1 = gsl_vector_get (model_parameters_x, 1);
    double p2 = gsl_vector_get (model_parameters_x, 2);
    double p3 = gsl_vector_get (model_parameters_x, 3);
    double p4 = gsl_vector_get (model_parameters_x, 4);
    double p5 = gsl_vector_get (model_parameters_x, 5);
    double p6 = gsl_vector_get (model_parameters_x, 6);
    double p7 = gsl_vector_get (model_parameters_x, 7);
    double p8 = gsl_vector_get (model_parameters_x, 8);

    // for each observation
    for (size_t i = 0; i < data_count; i++)
    {

        // evaluate the trend model at the data location
        double predicted_value_i =
                p0 * x[i]*x[i] +
                p1 * x[i]*y[i] +
                p2 * x[i]*z[i] +
                p3 * y[i]*y[i] +
                p4 * y[i]*z[i] +
                p5 * z[i]*z[i] +
                p6 * x[i] +
                p7 * y[i] +
                p8 * z[i];

        // compute and return the error between measurement and predicted.
        gsl_vector_set (residuals_at_data_locations, i, predicted_value_i - data_value[i]);
    }

    return GSL_SUCCESS;
}


//locally defined C-style function to compute the 1st derivative of
//the cost function in the form of a Jacobian matrix compatible with GSL's
//non-linear least-squares solver
int cost_df ( const gsl_vector * model_parameters_x,
              void *data,
              gsl_matrix * J ) {

    // get data information
    size_t data_count = static_cast<struct data *>(data)->count;
    double *x =         static_cast<struct data *>(data)->x;
    double *y =         static_cast<struct data *>(data)->y;
    double *z =         static_cast<struct data *>(data)->z;

    for (size_t i = 0; i < data_count; i++) {
        // An element of the Jacobian matrix: J(i,j) = ∂fi / ∂xj,
        // where fi = (Yi - yi)/sigma[i],
        //       Yi = predicted value at x,y,z ,
        //       yi = data value at x,y,z,
        //       xj = the nine trend model parameters p0...p8
        // f(x,y,z) = p0*x² + p1*x*y + p2*x*z + p3*y² + p4*y*z + p5*z² + p6*x + p7*y + p8*z
        gsl_matrix_set (J, i, 0, x[i]*x[i]); // ∂f/∂p0
        gsl_matrix_set (J, i, 1, x[i]*y[i]); // ∂f/∂p1
        gsl_matrix_set (J, i, 2, x[i]*z[i]); // ∂f/∂p2
        gsl_matrix_set (J, i, 3, y[i]*y[i]); // ∂f/∂p3
        gsl_matrix_set (J, i, 4, y[i]*z[i]); // ∂f/∂p4
        gsl_matrix_set (J, i, 5, z[i]*z[i]); // ∂f/∂p5
        gsl_matrix_set (J, i, 6, x[i]);      // ∂f/∂p6
        gsl_matrix_set (J, i, 7, y[i]);      // ∂f/∂p7
        gsl_matrix_set (J, i, 8, z[i]);      // ∂f/∂p8
    }

    return GSL_SUCCESS;
}


// locally defined C-style function that is called for each outer loop iteration of the non-linear
// least squares algorithm of GSL.
// this is usally useful to keep track of progress or to record an execution log
void iteration_callback( const size_t iter,
                         void *params,
                         const gsl_multifit_nlinear_workspace *w )
{
//  gsl_vector *f = gsl_multifit_nlinear_residual(w);
//  gsl_vector *x = gsl_multifit_nlinear_position(w);
//  double rcond;

//  /* compute reciprocal condition number of J(x) */
//  gsl_multifit_nlinear_rcond(&rcond, w);

//  fprintf(stderr, "iter %2zu: A = %.4f, lambda = %.4f, b = %.4f, cond(J) = %8.4f, |f(x)| = %.4f\n",
//          iter,
//          gsl_vector_get(x, 0),
//          gsl_vector_get(x, 1),
//          gsl_vector_get(x, 2),
//          1.0 / rcond,
//          gsl_blas_dnrm2(f));

    //update the progress bar (if set).
    if( s_progressDiag_for_iteration_callback ) {
        s_progressDiag_for_iteration_callback->setValue( iter );
        QApplication::processEvents(); //let Qt repaint widgets
    }
}

Quad3DTrendModelFittingAuxDefs::Parameters Quadratic3DTrendModelFitting::processWithNonLinearLeastSquares() const
{
    // load dataset
    m_dataFile->loadData();

    // set whether the data is 2D or 3D in Cartesian space
    bool is3D = m_dataFile->isTridimensional();

    // determine the number of valid samples (no NDV)
    size_t number_of_valid_samples = 0;
    {
        if( ! m_dataFile->hasNoDataValue() ){
            number_of_valid_samples = m_dataFile->getDataLineCount();
        } else {
            double NDV = m_dataFile->getNoDataValueAsDouble();
            int iColumn = m_attribute->getAttributeGEOEASgivenIndex() - 1;
            for( int iRow = 0; iRow < m_dataFile->getDataLineCount(); iRow++){
                double sample_value = m_dataFile->data( iRow, iColumn );
                if( ! Util::almostEqual2sComplement( NDV, sample_value, 1 ))
                    number_of_valid_samples++;
            }
        }
    }

    // select non-linear least squares method (so far GSL only supports TRS).
    // TRS = Trust Region Subproblem.  This means that in a small 9-D cube around a given
    //       x,y,z location, the trend model is assumed near-constant value.
    const gsl_multifit_nlinear_type* T = gsl_multifit_nlinear_trust;

    // set the sizes of the problem
    const size_t number_of_samples        = number_of_valid_samples;
    constexpr size_t number_of_parameters = Quad3DTrendModelFittingAuxDefs::N_PARAMS;

    // allocate data structures for algorithm operation
    double* x           = new double[number_of_samples];
    double* y           = new double[number_of_samples];
    double* z           = new double[number_of_samples];
    double* data_values = new double[number_of_samples];
    double* weights     = new double[number_of_samples];
    struct data dataSet = { number_of_samples, x, y, z, data_values };

    // GSL documentation is not very clear as to whatever this actually does but it is necessary.
    gsl_rng_env_setup();
    gsl_rng* trusted_region = gsl_rng_alloc( gsl_rng_default ); //the returned pointer is not being used by this client code

    // define the function to be minimized
    gsl_multifit_nlinear_fdf fdf;
    fdf.f = cost_f;
    fdf.df = cost_df;   // set to NULL for finite-difference Jacobian
    fdf.fvv = NULL;     // not using geodesic acceleration
    fdf.n = number_of_samples;
    fdf.p = number_of_parameters;
    fdf.params = &dataSet;

    // read the data to be fitted into the arrays
    {
        bool hasNDV = m_dataFile->hasNoDataValue();
        double NDV = m_dataFile->getNoDataValueAsDouble();
        int iColumn = m_attribute->getAttributeGEOEASgivenIndex() - 1;
        size_t iDataArray = 0;
        for( int iDataFileRow = 0; iDataFileRow < m_dataFile->getDataLineCount(); iDataFileRow++){
            double sample_value = m_dataFile->data( iDataFileRow, iColumn );
            if( ! hasNDV || ! Util::almostEqual2sComplement( NDV, sample_value, 1 )) {
                data_values[iDataArray] = sample_value;
                m_dataFile->getDataSpatialLocation( iDataFileRow, x[iDataArray], y[iDataArray], z[iDataArray] );
                weights[iDataArray] = 1.0;
                iDataArray++;
            }
        }
    }

    // from this point on, the data file's contents are no longer needed, so deallocate them
    // to free up memory space
    m_dataFile->freeLoadedData();

    // allocate workspace with default parameters
    gsl_multifit_nlinear_parameters fdf_params = gsl_multifit_nlinear_default_parameters();
    gsl_multifit_nlinear_workspace* workspace  = gsl_multifit_nlinear_alloc (T, &fdf_params, number_of_samples, number_of_parameters);

    // initialize solver with starting point and weights
    //                         0    1    2     3    4   5     6    7    8
    //                         x² + xy + xz +  y²+ yz + z² +  x +  y +  z
    double params_init[9] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 }; /* starting parameter values */
    if( ! is3D ) //zero-out z-bearing terms
        params_init[2] = params_init[4] = params_init[5] = params_init[8] = 0.0;
    gsl_vector_view parameters = gsl_vector_view_array (params_init, number_of_parameters);
    gsl_vector_view wts =        gsl_vector_view_array (weights,     number_of_samples);
    gsl_multifit_nlinear_winit (&parameters.vector, &wts.vector, &fdf, workspace);

    // compute initial cost function
    gsl_vector* f = gsl_multifit_nlinear_residual(workspace);
    double chisq0;
    gsl_blas_ddot(f, f, &chisq0);

    // perform the non-linear least squares of 100 iterations
    int info;
    constexpr int max_iterations = 100;
    constexpr double xtol = 1e-8; //parameter epsilon (e.g. 1.0 + epsilon == 1.0)
    constexpr double gtol = 1e-8; //1st derivative epsilon
    constexpr double ftol = 0.0;  //2nd derivative epsilon (not using geodesic acceleration)
    int status;
    {
        QProgressDialog progressDialog;
        progressDialog.setRange(0, max_iterations);
        progressDialog.setValue( 0 );
        progressDialog.show();
        progressDialog.setLabelText("Non-linear least squares in progress (max. " + QString::number(max_iterations) + " iterations) ...");
        s_progressDiag_for_iteration_callback = &progressDialog;
        status = gsl_multifit_nlinear_driver(max_iterations, xtol, gtol, ftol, iteration_callback, NULL, &info, workspace);
        s_progressDiag_for_iteration_callback = nullptr; //avoid dangling pointer after progressDialog goes out of scope
    }

    // get the final Jacobian matrix
    gsl_matrix* J = gsl_multifit_nlinear_jac(workspace);

    // compute the final covariance matrix of best fit parameters to get fit errors
    gsl_matrix* covar = gsl_matrix_alloc (number_of_parameters, number_of_parameters);
    gsl_multifit_nlinear_covar (J, 0.0, covar);

    // compute final cost value
    double chisq;
    gsl_blas_ddot(f, f, &chisq);

    // output execution summary
    Application::instance()->logInfo( "summary from method '" +
                                      QString(gsl_multifit_nlinear_name(workspace)) + "/" +
                                      QString(gsl_multifit_nlinear_trs_name(workspace)) + "'" );
    Application::instance()->logInfo( "number of iterations: " +
                                      QString::number(gsl_multifit_nlinear_niter(workspace)) );
    Application::instance()->logInfo( "function evaluations: " +
                                      QString::number(fdf.nevalf) );
    Application::instance()->logInfo( "Jacobian evaluations: " +
                                      QString::number(fdf.nevaldf) );
    Application::instance()->logInfo( "reason for stopping:: " +
                                      (info == 1 ? QString("small step size") : QString("small gradient")) );
    Application::instance()->logInfo( "initial |f(x)| = " +
                                      QString::number(sqrt(chisq0)) );
    Application::instance()->logInfo( "final   |f(x)| = " +
                                      QString::number(sqrt(chisq)) );

    // output the best fit trend model parameters with their fit errors
    {
        double dof = number_of_samples - number_of_parameters;
        double c = GSL_MAX_DBL(1, sqrt(chisq / dof));

        Application::instance()->logInfo( "chisq/dof = " +
                                          QString::number(chisq / dof) );

        // some lambda functions to serve as "macros"
        auto FIT = [workspace](size_t i){ return gsl_vector_get(workspace->x, i); };
        auto ERR = [covar](size_t i){ return std::sqrt(gsl_matrix_get(covar,i,i)); };

        //output the parameter values of the best fit trend model.
        for(size_t i = 0; i < Quad3DTrendModelFittingAuxDefs::N_PARAMS; i++ )
            Application::instance()->logInfo( "p" + QString::number(i) + " = " +
                                              QString::number(FIT(i)) + " +/- " +
                                              QString::number(c*ERR(i)) );
    }

    // output final execution status (e.g. whether it completed by convergence or
    // maximum number of iterations has been reached)
    Application::instance()->logInfo( "status = " +
                                      QString( gsl_strerror(status) ) );

    // free up memory
    gsl_multifit_nlinear_free (workspace);
    gsl_matrix_free (covar);
    gsl_rng_free (trusted_region);
    delete[] x;
    delete[] y;
    delete[] z;
    delete[] data_values;
    delete[] weights;

    // return best-fit trend model
    auto get_par = [workspace](size_t i){ return gsl_vector_get(workspace->x, i); };
    return Quad3DTrendModelFittingAuxDefs::Parameters( get_par(0),
                                                       get_par(1),
                                                       get_par(2),
                                                       get_par(3),
                                                       get_par(4),
                                                       get_par(5),
                                                       get_par(6),
                                                       get_par(7),
                                                       get_par(8) );
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
