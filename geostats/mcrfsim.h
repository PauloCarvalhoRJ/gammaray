#ifndef MCRFSIM_H
#define MCRFSIM_H

#include <QString>
#include <vector>
#include <mutex>
#include <random>

#include "spectral/spectral.h"
#include "geostats/searchstrategy.h"
#include "geostats/gridcell.h"
#include "geostats/taumodel.h"

class Attribute;
class CartesianGrid;
class CategoryPDF;
class VerticalTransiogramModel;
class CommonSimulationParameters;
class QProgressDialog;
class SpatialIndex;

/** Enum used to avoid the slow File::getFileType() in performance-critical code. */
enum class PrimaryDataType : int {
    UNDEFINED,
    POINTSET,
    CARTESIANGRID,
    GEOGRID,
    SEGMENTSET
};

/** Enum used for clarity when addressing the source index in Tau Model.
 * The values are used as array indexes, so they must start at 0 and increase by 1.
 */
enum class ProbabilitySource : unsigned int {
    FROM_TRANSIOGRAM = 0,
    FROM_SECONDARY_DATA = 1
};

/** Enum used to switch execution mode between normal and for Bayesian application.
 *  Bayesian mode allows certain hyperparameters (e.g. Tau Model factors) to vary
 *  randomly in a given range or amongst a predefined set from realization to realization.
 *  Bayesian mode accounts for the hyperparameter uncertainty in the realizations.
 */
enum class MCRFMode : unsigned int {
    NORMAL = 0,  /** Hyperparameters do not vary. */
    BAYESIAN = 1 /** Some hyperparameters are drawn from an interval or set for each realization. */
};

/** A multithreaded implementation of the Markov Chains Random Field Simulations with secondary data and
 * probability integration with the Tau Model.  This algorithm uses the Mersenne Twister pseudo-random generator
 * of 32-bit numbers with a state size of 19937 bits implemented as C++ STL's std::mt19937 class to generate its
 * random path and Monte Carlo draw.
 *
 * ATTENTION: The methods named *MT() are called from multiple threads.
 *
 * REF: Markov Chain Random Fields for Estimation of Categorical Variables.
 *      Weidong Li, Math Geol (2007), 39: 321-335
 *      DOI: 10.1007/s11004-007-9081-0
 */
class MCRFSim
{

public:
    MCRFSim( MCRFMode mode );

    /**
     * \defgroup MCRFSimParameters The simulation parameters.
     */
    /*@{*/
    /** The categorical attribute to simulate. */
    Attribute* m_atPrimary;
    /** The attribute of the input data set that contains the gradation field values.  See the program manual for
     *  detailed explanation on the role of the gradation field. */
    Attribute*                m_gradationFieldOfPrimaryData;
    std::vector< Attribute* > m_gradationFieldsOfPrimaryDataBayesian; //for Bayesian mode, there can be multiple gradation fields.
    /** The primary data set. */
    DataFile* m_dfPrimary;
    /** The simulation grid. */
    CartesianGrid* m_cgSim;
    /** The PDF with the target global distribution of facies. */
    CategoryPDF* m_pdf;
    /** The vertical transiogram model. */
    VerticalTransiogramModel* m_transiogramModel;
    /** A 2nd vertical transiogram model to make out a transiogram band of uncertainity for Bayesian mode. */
    VerticalTransiogramModel* m_transiogramModel2Bayesian;
    /** The attribute of the simulation grid that contains the gradation field.  See the program manual for
     *  detailed explanation on the role of the gradation field. */
    Attribute*                m_gradationFieldOfSimGrid;
    std::vector< Attribute* > m_gradationFieldsOfSimGridBayesian; //for Bayesian mode, there can be multiple gradation fields.
    /** The optional probability fields (attributes of the simulation grid).
     * An empty vector means no probability field will be used.  Otherwise, the fields must match the number and order
     * of categories as present in the m_atPrimary's CategoryDefinition object.
     */
    std::vector< Attribute* > m_probFields;
    /** For Bayesian mode: The optional probability fields (attributes of the simulation grid).
     * Inner vector: probability fields from which one is drawn in a realization for one given category.
     * Outer vector: one inner vector for each category.
     * An empty inner vector means no probability field will be used.  The number of fields must be the same
     * accross the inner vectors (all categories must be given the same number or probability fields).
     * The number of elements in the outer vector must match the number and order
     * of categories as present in the m_atPrimary's CategoryDefinition object.
     */
    std::vector< std::vector< Attribute* > > m_probsFieldsBayesian;
    /** The Tau factor for the probabilities given by the transiogram model. */
    double m_tauFactorForTransiography;
    double m_tauFactorForTransiographyBayesianStarting; //lower limit for values to be drawn (Bayesian mode).
    double m_tauFactorForTransiographyBayesianEnding;   //upper limit for values to be drawn (Bayesian mode).
    /** The Tau factor for the probabilities given by the secondary data. */
    double m_tauFactorForProbabilityFields;
    double m_tauFactorForProbabilityFieldsBayesianStarting; //lower limit for values to be drawn (Bayesian mode).
    double m_tauFactorForProbabilityFieldsBayesianEnding;   //upper limit for values to be drawn (Bayesian mode).
    /** The common simulation parameters (e.g. random seed number, number of realizations, search parameters, etc.) */
    CommonSimulationParameters* m_commonSimulationParameters;
    /** Sets whether the algorithm should invert the gradation field convention. */
    bool m_invertGradationFieldConvention;
    /** Sets the maximum number of threads the simulation will execute in. */
    uint m_maxNumberOfThreads;
    /*@}*/

    /** Runs the algorithm.  If false is returned, the simulation failed.  Call getLastError()
     * to obtain the reasons.
     */
    bool run();

    /** Returns a text explaining the cause of the last failure during the simulation. */
    QString getLastError() const{ return m_lastError; }

    /** Simulates one cell.
     * It retuns a double because the double is the basic data element in DataFile object
     * even though one expect just integers (category codes) in a Markov Chain Simulation.
     * The simulation may result in no-data-value, which is not necessarily an integer.
     * ATTENTION: this method may be called from multiple threads, so be careful when
     *            writing to objects other than "this" here.  So it is advisable to call
     *            const methods or encase non-const ones in mutex locks (degrades performance).
     * NOTE ON PERFORMANCE: this method has potential to be called a billion times, so optmization
     *                      is critical in every line of code of this method as well as of every
     *                      method it calls.
     * @param i Topologic coordinate of the cell to simulate.
     * @param j Topologic coordinate of the cell to simulate.
     * @param k Topologic coordinate of the cell to simulate.
     * @param randomNumberGenerator The random number generator ( one per thread is advisable ).
     * @param tauFactorForTransiography The Tau model factor to be used for the probabilities given by transiography.
     * @param tauFactorForSecondaryData The Tau model factor to be used for the probabilities given by secondary
     *                                  data (probabiliy fields for each category).
     * @param gradFieldOfPrimaryDataToUse The gradation field variable of the primary data set to use.
     * @param gradFieldOfSimGridToUse The gradation field variable of the simulation grid to use.
     * @param transiogramToUse The vertical transiogram model to use.
     * @param probFields The set of probability fields to use.  Must be one for each category and must match
     *                   the order of the categories as present in the m_atPrimary's CategoryDefinition object.
     * @param simulatedData Pointer to the realization data so it is possible to retrieve the previously
     *                      simulated values.
     */
    double simulateOneCellMT( uint i, uint j , uint k,
                              std::mt19937& randomNumberGenerator,
                              double tauFactorForTransiography,
                              double tauFactorForSecondaryData,
                              const Attribute* gradFieldOfPrimaryDataToUse,
                              const Attribute* gradFieldOfSimGridToUse,
                              const VerticalTransiogramModel &transiogramToUse,
                              const std::vector<Attribute *> &probFields,
                              const spectral::array& simulatedData ) const;

    /** Sets or increases the current simulation progress counter to the given ammount.
     * Mind that this function updates a progress bar, which is a costly operation.
     * @note Despite being non-const, this method contains a critical section, so it is safe to
     *       call from multiple threads.
     */
    void setOrIncreaseProgressMT( ulong ammount, bool increase = true );

    /** Saves a realization's simulated data.  Depending on user's settings, the realizations
     * are saved as:
     * 1) New attributes in the simulation grid.
     * 2) New cartesian grids with one attribute in the project.
     * 3) New cartesian grid files with one attribute saved in some directory outside the project.
     * The parameters with names ending in *Used are the paramaters and hyperparameters used.  These
     * vary when execution mode is Bayesian and are saved to a report file useful for data anaysis.
     * @note Despite being non-const, this method contains a critical section, so it is safe to
     *       call from multiple threads.
     */
    void saveRealizationMT(const spectral::arrayPtr simulatedData,
                            VerticalTransiogramModel &transiogramUsed,
                            const std::vector<Attribute *> &probFieldsUsed,
                            const Attribute *gradFieldOfSimGridUsed,
                            const Attribute *gradFieldOfPrimaryDataUsed,
                            double tauFactorForTransiographyUsed,
                            double tauFactorForSecondaryDataUsed );

    /** Returns the execution mode (normal or for Bayesian application) for this simulation.
     * @see MCRFMode
     */
    MCRFMode getMode() const;

    /** Sets the execution mode (normal or for Bayesian application) for this simulation.
     * @see MCRFMode
     */
    void setMode(const MCRFMode &mode);

    /** This method is normally called in the program's main() when the user wants to run MCRF
     * without interacting with dialogs. In this case, the user provides a path to a
     * complete configuration file.
     * @return The status code to be returned in main(). Normally it should be zero for a
     *         successful run and non-zero for error in execution.
     */
    static int runUnattended();

private:
    /** The simulation execution mode.
     * NORMAL  : hyperparameters (e.g. Tau Model factors) remain the same across realizations.
     * BAYESIAN: some hyperparameters randomly vary across realizations.
     */
    MCRFMode m_mode;

    /** The description of the cause of the last failure during simulation. */
    QString m_lastError;

    //!@{
    //! Objects used in the progress bar updating during multithreaded execution.
    std::mutex m_mutexMCRF;
    QProgressDialog* m_progressDialog;
    ulong m_progress;
    //!@}

    //!@{
    //! Objects used in realization saving during multithreaded execution to prevent
    //! crashes and inconsistent project object state.
    std::mutex m_mutexSaveRealizations;
    //!@}

    /** The simulation grid's no-data-value as a double value to avoid unnecessary iterative calls to DataFile::getNoDataValue*(). */
    double m_simGridNDV;

    /** The sprimary data's no-data-value as a double value to avoid unnecessary iterative calls to DataFile::getNoDataValue*(). */
    double m_primaryDataNDV;
    bool m_primaryDataHasNDV;

    //!@{
    //! The search strategies for the primary data and the simulation grid.
    SearchStrategyPtr m_searchStrategyPrimary;
    SearchStrategyPtr m_searchStrategySimGrid;
    //!@}

    //!@{
    //! The spatial indexes for the primary data and the simulation grid.
    std::shared_ptr<SpatialIndex> m_spatialIndexOfPrimaryData;
    std::shared_ptr<SpatialIndex> m_spatialIndexOfSimGrid;
    //!@}

    /** An enum value to avoid iterative calls to slow File::getFileType(). */
    PrimaryDataType m_primaryDataType;

    /** The input data. */
    DataFile* m_primaryDataFile;

    /** The Tau Model used to integrate different sources of facies probabilities. */
    TauModelPtr m_tauModel;

    /**
     * The realization number.  This value starts with 1 in the constructor and is incremented
     * when a realization is saved.
     */
    uint m_realNumberForSaving;

    /** Returns whether the simulation parameters are valid and consistent. */
    bool isOKtoRun();

    /** Returns whether the simulation will use collocated facies probability fields. */
    bool useSecondaryData() const;

    /** Causes the progress window to repaint (slows down execution if called many times unnecessarily). */
    void updateProgessUI();

    /** Returns a container with the primary data samples around the estimation cell to be used in the estimation.
     * The resulting collection depends on the SearchStrategy object set for the primary data.  Returns an empty object if any
     * required parameter for the search to work (e.g. input data) is missing.  The data cells are ordered
     * by their distance to the passed simulation cell.
     */
    DataCellPtrMultiset getSamplesFromPrimaryMT( const GridCell& simulationCell ) const;

    /** Returns a container with the simulation grid cells around the estimation cell.
     * The resulting collection depends on the SearchStrategy object set for the simulation grid.  Returns an empty object if any
     * required parameter for the search to work is missing.  The data cells are ordered
     * by their distance to the passed simulation cell.
     * This method also needs to query the previously simulated data, which is passed as a parameter.
     */
    DataCellPtrMultiset getNeighboringSimGridCellsMT(const GridCell& simulationCell ,
                                                     const spectral::array &simulatedData) const;

    /** Returns the path to the report file containing the transiogram paramaters and hyperparameters
     * used in each realization.  These vary when the simulation executes in Bayesian mode.
     */
    QString getReportFilePathForBayesianModeMT() const;

};

#endif // MCRFSIM_H
