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
    MCRFSim();

    /**
     * \defgroup MCRFSimParameters The simulation parameters.
     */
    /*@{*/
    /** The categorical attribute to simulate. */
    Attribute* m_atPrimary;
    /** The attribute of the input data set that contains the gradation field values.  See the program manual for
     *  detailed explanation on the role of the gradation field. */
    Attribute* m_gradationFieldOfPrimaryData;
    /** The primary data set. */
    DataFile* m_dfPrimary;
    /** The simulation grid. */
    CartesianGrid* m_cgSim;
    /** The PDF with the target global distribution of facies. */
    CategoryPDF* m_pdf;
    /** The vertical transiogram model. */
    VerticalTransiogramModel* m_transiogramModel;
    /** The attribute of the simulation grid that contains the gradation field.  See the program manual for
     *  detailed explanation on the role of the gradation field. */
    Attribute* m_gradationFieldOfSimGrid;
    /** The optional probability fields (attributes of the simulation grid).
     * An empty vector means no probability field will be used.  Otherwise, the fields must match the number and order
     * of categories as present in the m_atPrimary's CategoryDefinition object.
     */
    std::vector< Attribute* > m_probFields;
    /** The Tau factor for the probabilities given by the transiogram model. */
    double m_tauFactorForTransiography;
    /** The Tau factor for the probabilities given by the secondary data. */
    double m_tauFactorForProbabilityFields;
    /** The common simulation parameters (e.g. random seed number, number of realizations, search parameters, etc.) */
    CommonSimulationParameters* m_commonSimulationParameters;
    /** Sets whether the algorithm should invert the gradation field convention. */
    bool m_invertGradationFieldConvention;
    /** Sets the maximum number of threads the simulation will execute in. */
    uint m_maxNumberOfThreads;
    /*@}*/

    /** Runs the algorithm.  If false is returned, the simulation failed.  Call getLastError() to obtain the reasons. */
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
     * @param simulatedData Pointer to the realization data so it is possible to retrieve the previously
     *                      simulated values.
     */
    double simulateOneCellMT( uint i, uint j , uint k,
                              std::mt19937& randomNumberGenerator, const spectral::array& simulatedData ) const;

    /** Sets or increases the current simulation progress counter to the given ammount.
     * Mind that this function updates a progress bar, which is a costly operation.
     */
    void setOrIncreaseProgressMT( ulong ammount, bool increase = true );

    /** Returns the realizations simulated in the last sucessful call to run().
     * Each spectral::array object is a series of double values that match
     * the scan order of the simulation grid.  Values matching the simulation grid's
     * no-data-value are uninformed values.
     */
    const std::vector< spectral::arrayPtr >& getRealizations() const { return m_realizations; }

private:

    /** The description of the cause of the last failure during simulation. */
    QString m_lastError;

    //!@{
    //! Objects used in the progress bar updating during multithreaded execution.
    std::mutex m_mutexMCRF;
    QProgressDialog* m_progressDialog;
    ulong m_progress;
    //!@}

    /** The simulation grid's no-data-value as a double value to avoid unnecessary iterative calls to DataFile::getNoDataValue*(). */
    double m_simGridNDV;

    /** The sprimary data's no-data-value as a double value to avoid unnecessary iterative calls to DataFile::getNoDataValue*(). */
    double m_primaryDataNDV;
    bool m_primaryDataHasNDV;

    /** The set of simulated data.  Each spectral::array object is a string of doubles that matches the
     * scan order of the simulation grid.
     */
    std::vector< spectral::arrayPtr > m_realizations;

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
     */
    DataCellPtrMultiset getNeighboringSimGridCellsMT( const GridCell& simulationCell ) const;

};

#endif // MCRFSIM_H
