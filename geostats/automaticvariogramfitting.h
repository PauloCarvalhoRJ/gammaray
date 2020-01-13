#ifndef AUTOMATICVARIOGRAMFITTING_H
#define AUTOMATICVARIOGRAMFITTING_H

#include <QObject> //to enable Qt's signals/slots mechanism
#include "spectral/spectral.h"
#include "imagejockey/ijvariographicmodel2d.h"
#include "geostats/nestedvariogramstructuresparameters.h"

class Attribute;
class CartesianGrid;
class IJGridViewerWidget;
class IJAbstractCartesianGrid;

/** The parameters domain for the optimization methods (bound conditions). */
struct VariogramParametersDomain {
    IJVariographicStructure2D min;
    IJVariographicStructure2D max;
};

/*! The method for fast variogram map computing. */
enum class FastVarmapMethod : int{
    VARMAP_WITH_FIM,     /*!< Compute with the Fourier Integral Method. */
    VARMAP_WITH_SPECTRAL /*!< Compute with a spectral method (not scientifically validated yet). */
};

/*! The type of objective function. */
enum class ObjectiveFunctionType : int{
    BASED_ON_FIM,     /*!< Compares the input map with the map sinthetized from the variogram model with FIM. */
    BASED_ON_VARFIT   /*!< Compares the varmap of input with the theoretic variogram surface. */
};

/** Performs full 2D automatic variogram fitting for data in regular grids . */
class AutomaticVariogramFitting : public QObject
{

    Q_OBJECT

public:
    explicit AutomaticVariogramFitting( Attribute* at );
    ~AutomaticVariogramFitting();

    /** Sets the method for fast variogram map computing. */
    void setFastVarmapMethod( FastVarmapMethod fastVarmapMethod );

    /** The objective function for the optimization processes.
     * @param gridWithGeometry A grid object whose geometry will be copied to the generated grid with the varmap for objective function evaluation.
     * @param inputData The grid data for comparison.
     * @param vectorOfParameters The column-vector with the free paramateres (variogram parameters).
     * @param m The desired number of variographic nested structures.
     * @return A distance/difference measure.
     */
    double objectiveFunction ( const IJAbstractCartesianGrid &gridWithGeometry,
                               const spectral::array &inputGridData,
                               const spectral::array &vectorOfParameters,
                               const int m ) const;

    /**
     * @brief Moves a point (a solution) along a line for the Line Search with Restart optimization.
     * @param m Number of variogram nested structures.
     * @param i Point index.
     * @param k Optimization step number. First must be 1, not 0.
     * @param domain The min/max variogram parameters boundaries as an object.
     * @param L_wMax The min variogram parameters boundaries as a linear array.
     * @param L_wMin The max variogram parameters boundaries as a linear array.
     * @param inputGrid The grid object with grid geometry.
     * @param inputData The grid input data.
     * @param randSequence Sequence of values returned by std::rand()/(double)RAND_MAX calls made before hand.  Its number of elements must be
     *                     Number of optimization steps * startingPoints.size() * vw_bestSolution.size()
     *                     A prior random number generation is to preserve the same random walk for a given seed
     *                     independently of number and order of multiple threads execution.
     * OUTPUT PARAMETERS:
     * @param startingPoints The set of points (solutions) that will travel along lines.
     * @param fOfBestSolution The value of objetive function at the best solution found.
     * @param vw_bestSolution The variogram parameters of the best solution found (as linear array).
     */
    void movePointAlongLineForLSRS(int m,
                           int i,
                           int k,
                           const VariogramParametersDomain& domain,
                           const spectral::array &L_wMax,
                           const spectral::array &L_wMin,
                           const IJAbstractCartesianGrid &inputGrid,
                           const spectral::array &inputData,
                           const spectral::array &randSequence,
                           std::vector<spectral::array> &startingPoints,
                           double &fOfBestSolution,
                           spectral::array &vw_bestSolution) const;

    /**
     * Returns the FFT phase map of the input data.
     */
    spectral::array getInputPhaseMap() const;

    /** Computes the varmap of the input data.
     * TODO: consider moving this method to either CartesianGrid,
     * IJAbstractCartesianGrid, GridFile or Util. */
    spectral::array computeVarmap() const;

    /** Method called by the several methods of optimization to initialize the
     *  parameter domain (boundary conditions) and the set of parameters.
     * All non-const method parameters are output parameters.
     * @param inputVarmap The varmap of the input grid data.
     * @param m The number of wanted variogram nested structures.
     * @param domain The domain boundaries as a VariogramParamateresDomainObject
     * @param vw The set of parameters as a linear array.
     * @param L_wMin The lower parameters boundaries as a linear array.
     * @param L_wMax The upper parameters boundaries as a linear array.
     * @param variogramStructures The set of parameters as a structured object.
     */
    void initDomainAndParameters( const spectral::array &inputVarmap,
                                  int m,
                                  VariogramParametersDomain& domain,
                                  spectral::array& vw,
                                  spectral::array& L_wMin,
                                  spectral::array& L_wMax,
                                  std::vector<IJVariographicStructure2D> &variogramStructures ) const;

    /** Method called to display a dialog with relevant results from the variogram structures passed.
     * This method is not particularly useful to any client code.  It is used internally to reuse code.
     * @param variogramStructures The nested variogram structures as parameters (azimuth, axis, etc.).
     * @param fftPhaseMapOfInput The FFT phase map of the input grid data.
     * @param varmapOfInput Varmap fo the input grid data.
     * @param modal If true, the dialog blocks execution until it is closed.
     */
    void displayResults(const std::vector< IJVariographicStructure2D >& variogramStructures ,
                        const spectral::array &fftPhaseMapOfInput,
                        const spectral::array &varmapOfInput,
                        bool modal ) const;

    /** Sets the type of objective function for optimization. */
    void setObjectiveFunctionType( ObjectiveFunctionType objectiveFunctionType ){
        m_objectiveFunctionType = objectiveFunctionType;
    }

    /** Rreturns the series of objective function values generated in the last run
     * of either of the optimization algorithms.
     */
    static const std::vector< double >& getObjectiveFunctionValuesOfLastRun() {
        return s_objectiveFunctionValues;
    }

private:
    Attribute* m_at;
    CartesianGrid* m_cg;
    FastVarmapMethod m_fastVarmapMethod;
    ObjectiveFunctionType m_objectiveFunctionType;

    /** The objective function values collected during the last execution
     * of an optimization method.
     */
    static std::vector< double > s_objectiveFunctionValues;

    /** Utilitary function that encapsulates variographic surface generation from
     * variogram model parameters.
     * @param gridWithGeometry A grid object whose geometry will be copied to the generated grid.
     * @param vectorOfParameters The vector with the variographic parameters follwoing this order:
     *                           [axis0,ratio0,az0,cc0,axis1,ratio1,...].
     * @param m The number of structures.
     * TODO: consider moving this method to GeostatsUtil.
     */
    spectral::array generateVariographicSurface( const IJAbstractCartesianGrid& gridWithGeometry,
                                                 const spectral::array &vectorOfParameters,
                                                 const int m ) const;

    /**
     * Displays a series of grids in a dialog.
     * TIP C++11: use displayGrids({A}, {"A matrix"}, {false}); to display a single grid.
     * @param modal If true, the dialog blocks execution until it is closed.
     */
    void displayGrids( const std::vector< spectral::array >& grids,
                       const std::vector< std::string >& titles,
                       const std::vector< bool >& shiftByHalves,
                       bool modal ) const;

    /**
     * Applies the principle of the Fourier Integral Method to obtain the map in spatial domain
     * from a variographic map (theoretical or experimental) and a map of FFT phases.
     * REF: The Fourier Integral Method: An Efficient Spectral Method For Simulation of Random Fields. (Pardo-Iguzquiza & Chica-Olmo, 1993)
     * @param gridWithCovariance The grid data with the variographic surface.  It must be actually a correlographic surface,
     *                           that is, with max value at the center and decrasing with distance from the center.
     *                           h=0 must be at the center of the grid.
     * @param gridWithFFTphases The grid data with the FFT phases in radians.  Must vary between -PI and +PI.
     */
    spectral::array computeFIM( const spectral::array& gridWithCovariance,
                                const spectral::array& gridWithFFTphases ) const;


    /**
     * Displays a dialog with a chart showing the evolution of the objective function
     * value versus iterations of the optimization method.
     */
    void showObjectiveFunctionEvolution( ) const;

    /** The specialization of objectiveFunction() that follows the objective function proposed by
     * Larrondo et al (2003) - VARFIT: A Program for Semi-Automatic Variogram Modelling
     *                         Center for Computational Geostatistics Report Five
     * See complete theory in the program manual for in-depth explanation of the method's parameters below.
     * @param gridWithGeometry A grid object whose geometry will be copied to the generated grid with the varmap for objective function evaluation.
     * @param inputData The grid data for comparison.
     * @param vectorOfParameters The column-vector with the free paramateres (variogram parameters).
     * @param m The desired number of variographic nested structures.
     * @return A distance/difference measure.
     */
    double objectiveFunctionVARFIT ( const IJAbstractCartesianGrid &gridWithGeometry,
                                     const spectral::array &inputGridData,
                                     const spectral::array &vectorOfParameters,
                                     const int m ) const;

    /** The specialization of objectiveFunction() that compares the input map with the map sinthetized
     * from the theoretical variogram with the Fourier Integral Method.
     * See complete theory in the program manual for in-depth explanation of the method's parameters below.
     * @param gridWithGeometry A grid object whose geometry will be copied to the generated grid with the varmap for objective function evaluation.
     * @param inputData The grid data for comparison.
     * @param vectorOfParameters The column-vector with the free paramateres (variogram parameters).
     * @param m The desired number of variographic nested structures.
     * @return A distance/difference measure.
     */
    double objectiveFunctionFIM ( const IJAbstractCartesianGrid &gridWithGeometry,
                                  const spectral::array &inputGridData,
                                  const spectral::array &vectorOfParameters,
                                  const int m ) const;

public:

    /** Performs automatic variogram fitting using Simulated Annealing and Gradient Descent in tandem
     *  as optimization method.
     *...................................Global Parameters....................................
     * @param nThreads Number of parallel execution threads.
     * @param m Number of variogram structures to fit.
     * @param seed Seed for the the random number generator.
     *...................................Annealing Parameters.................................
     * @param f_Tinitial Intial temperature.
     * @param f_Tfinal Final temperature.
     * @param i_kmax Max number of SA steps.
     * @param f_factorSearch Factor used to control the size of the random state “hop”.  For example, if the maximum “hop” must be
     *                       10% of domain size, set 0.1.  Small values (e.g. 0.001) result in slow, but more accurate convergence.
     *                       Large values (e.g. 100.0) covers more space faster, but falls outside the domain are more frequent,
     *                       resulting in more re-searches due to more invalid parameter value penalties.
     *.................................Gradient Descent Parameters............................
     * @param maxNumberOfOptimizationSteps Max number of GD steps.
     * @param epsilon User-given epsilon (useful for numerical calculus).  It is normally a small value (e.g.: 10^-6)
     * @param initialAlpha Alpha is the factor by which the gradient value is multiplied.
     *                     a small value prevents overshooting.
     * @param maxNumberOfAlphaReductionSteps Alpha is reduced iteratively until a descent is detected (no overshoot).
     * @param convergenceCriterion GD stops after two consecutive steps yield two objective function values whose
     *                             difference is less than this value.  It is normally a small value (e.g.: 10^-6)
     * @param openResultsDialog If true, the results are also displayed in a dialog.
     * @returns The fitted variogram model as a vector of variographic structures.
     */
    std::vector< IJVariographicStructure2D > processWithSAandGD(
                            unsigned int nThreads,
                            int m,
                            unsigned seed,
                            double f_Tinitial,
                            double f_Tfinal,
                            int i_kmax,
                            double f_factorSearch,
                            int maxNumberOfOptimizationSteps,
                            double epsilon,
                            double initialAlpha,
                            double maxNumberOfAlphaReductionSteps,
                            double convergenceCriterion,
                            bool openResultsDialog = true
                            ) const;

    /** Performs automatic variogram fitting using Line Search with Restart
     *  as optimization method.
     *...................................Global Parameters....................................
     * @param nThreads Number of parallel execution threads.
     * @param m Number of variogram structures to fit.
     * @param seed Seed for the the random number generator.
     *...................................LSRS Parameters.................................
     * @param maxNumberOfOptimizationSteps Number of iterations.
     * @param epsilon The user-given epsilon (useful for numerical calculus). It is normally a small value (e.g.: 10^-6)
     * @param nStartingPointsN umber of random starting points in the domain.
     * @param nRestarts Number of restarts.
     * @param openResultsDialog If true, the results are also displayed in a dialog.
     * @returns The fitted variogram model as a vector of variographic structures.
     */
    std::vector< IJVariographicStructure2D > processWithLSRS(
                         unsigned int nThreads,
                         int m,
                         unsigned seed,
                         int maxNumberOfOptimizationSteps,
                         double epsilon,
                         int nStartingPoints,
                         int nRestarts,
                         bool openResultsDialog = true
            ) const;

    /** Performs automatic variogram fitting using Particle Swarm Optimization
     *  as optimization method.
     *...................................Global Parameters....................................
     * @param m Number of variogram structures to fit.
     * @param seed Seed for the the random number generator.
     *.....................................PSO Parameters.....................................
     * @param maxNumberOfOptimizationSteps Number of iterations.
     * @param nParticles Number of wandering particles.
     * @param intertia_weight The inertia of the particles.  Greater values means less velocity variation.
     * @param acceleration_constant_1 The acceleration caused by the particle's best evaluation location.
     * @param acceleration_constant_2 The acceleration caused by the global best evaluation location.
     * @param openResultsDialog If true, the results are also displayed in a dialog.
     * @returns The fitted variogram model as a vector of variographic structures.
     */
    std::vector< IJVariographicStructure2D > processWithPSO(
                        int m,
                        unsigned seed,
                        int maxNumberOfOptimizationSteps,
                        int nParticles,
                        double intertia_weight,
                        double acceleration_constant_1,
                        double acceleration_constant_2,
                        bool openResultsDialog = true
            ) const;

    /** Performs automatic variogram fitting using Genetic Algorithm
     *  as optimization method.
     *...................................Global Parameters....................................
     * @param nThreads Number of parallel execution threads.
     * @param m Number of variogram structures to fit.
     * @param seed Seed for the the random number generator.
     *...................................GA Parameters.................................
     * @param maxNumberOfGenerations number of generations (iterations).
     * @param nPopulationSize Number of individuals (solutions).
     * @param nSelectionSize The size of the selection pool (must be < nPopulationSize).
     * @param probabilityOfCrossOver The probability of crossover (between 0.0 and 1.0);
     * @param pointOfCrossover The parameter index from which crossover takes place (must be less than the total
     *                         number of parameters per individual).
     * @param mutationRate Mutation rate means how many paramaters are expected to change per mutation
     *                     the probability of any parameter parameter (gene) to be changed is 1/nParameters * mutationRate
     *                     thus, 1.0 means that one gene will surely be mutated per mutation on average.  Fractionary
     *                     values are possible. 0.0 means no mutation will take place.
     * @param openResultsDialog If true, the results are also displayed in a dialog.
     * @returns The fitted variogram model as a vector of variographic structures.
     */
    std::vector< IJVariographicStructure2D > processWithGenetic(
                            int nThreads,
                            int m,
                            unsigned seed,
                            int maxNumberOfGenerations,
                            uint nPopulationSize,
                            uint nSelectionSize,
                            double probabilityOfCrossOver,
                            uint pointOfCrossover,
                            double mutationRate,
                            bool openResultsDialog = true
            ) const;

    /** Evaluates the objective function for the passed variogram model as a vector of variogram structures.
     */
    double evaluateModel( const std::vector< IJVariographicStructure2D >& variogramStructures ) const;

private Q_SLOTS:

    void onSaveAResult( spectral::array* result );
};


#endif // AUTOMATICVARIOGRAMFITTING_H
