#ifndef MCRFSIM_H
#define MCRFSIM_H

#include <QString>
#include <vector>
#include <mutex>

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

/** This enums controls how the vertical transiography is translated to lateral transiography. */
enum class LateralGradationType : int {
    TAIL_TRANSIOGRAMS_ONLY = 0,            /*!< Facies to be simulated is always prior to a known/simulated location in lateral sequence. */
    HEAD_TRANSIOGRAMS_ONLY,                /*!< Facies to be simulated is always posterior to a known/simulated location in lateral sequence. */
    HEAD_AND_TAIL_TRANSIOGRAMS_AT_RANDOM,  /*!< Facies to be simulated can be randomly prior or posterior to a known/simulated location in lateral sequence. */
    USE_GRADATIONAL_FIELD,                 /*!< Uses an attribute in the simulation grid to compute the separation from for the lateral transiography instead
                                                of transiogram ranges (vertical range and the LVA fields).
                                                E.g.: two locations with the same value have zero separation stratigraphically speaking, even if they
                                                are far apart in space.*/
};

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

/** A mulithreaded implementation of the Markov Chains Random Field Simulations with secondary data and
 * probability integration with the Tau Model.  This algorithm uses the Mersenne Twister pseudo-random generator
 * of 32-bit numbers with a state size of 19937 bits implemented as C++ STL's std::mt19937 class to generate its
 * random path and Monte Carlo draw.
 *
 * ATTENTION: The methods named *MT() run from multiple threads.
 *
 * REF: Markov Chai Random Fields for Estimation of Categorical Variables.
 *      Weidong Li, Math Geol (2007), 39: 321-335
 *      DOI: 10.1007/s11004-007-9081-0
 */
class MCRFSim
{

public:
    MCRFSim();

    /** The categorical attribute to simulate. */
    Attribute* m_atPrimary;

    /** The simulation grid. */
    CartesianGrid* m_cgSim;

    /** The PDF with the target global distribution of facies. */
    CategoryPDF* m_pdf;

    /** The vertical transiogram model. */
    VerticalTransiogramModel* m_transiogramModel;

    /** How to translate vertical transiography to lateral. See documentation of the LateralGradationType enum. */
    LateralGradationType m_lateralGradationType;

    /** The optional attribute of the simulation grid that contains the gradation field. See documentation of the LateralGradationType enum. */
    Attribute* m_gradationField;

    /** The optional attribute of the simulation grid that contains the locally varying anisotropy azimuth.
     * See documentation of the LateralGradationType enum. */
    Attribute* m_LVAazimuth;

    /** The optional attribute of the simulation grid that contains the locally varying anisotropy semi-major axis.
     * See documentation of the LateralGradationType enum. */
    Attribute* m_LVAsemiMajorAxis;

    /** The optional attribute of the simulation grid that contains the locally varying anisotropy semi-minor axis.
     * See documentation of the LateralGradationType enum. */
    Attribute* m_LVAsemiMinorAxis;

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

    /** Runs the algorithm.  If false is returned, the call failed.  Call getLastError() to obtain the reasons. */
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
     */
    double simulateOneCellMT(uint i, uint j , uint k) const;

    /** Sets or increases the current simulation progress counter to the given ammount.
     * Mind that this function updates a progress bar, which is a costly operation.
     */
    void setOrIncreaseProgressMT( ulong ammount, bool increase = true );

private:

    QString m_lastError;

    std::mutex m_mutexMCRF;
    QProgressDialog* m_progressDialog;
    ulong m_progress;

    double m_simGridNDV;

    std::vector< spectral::arrayPtr > m_realizations;

    SearchStrategyPtr m_searchStrategyPrimary;
    SearchStrategyPtr m_searchStrategySimGrid;

    std::shared_ptr<SpatialIndex> m_spatialIndexOfPrimaryData;
    std::shared_ptr<SpatialIndex> m_spatialIndexOfSimGrid;

    PrimaryDataType m_primaryDataType;

    DataFile* m_primaryDataFile;

    TauModelPtr m_tauModel;

    bool isOKtoRun();

    bool useSecondaryData() const;

    void updateProgessUI();

    /** Returns a container with the primary data samples around the estimation cell to be used in the estimation.
     * The resulting collection depends on the SearchStrategy object set.  Returns an empty object if any
     * required parameter for the search to work (e.g. input data) is missing.  The data cells are ordered
     * by their distance to the passed simulation cell.
     */
    DataCellPtrMultiset getSamplesFromPrimaryMT( const GridCell& simulationCell ) const;

    DataCellPtrMultiset getNeighboringSimGridCellsMT( const GridCell& simulationCell ) const;
};

#endif // MCRFSIM_H
