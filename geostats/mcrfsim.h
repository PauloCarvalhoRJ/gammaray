#ifndef MCRFSIM_H
#define MCRFSIM_H

#include <vector>
#include <QString>

class Attribute;
class CartesianGrid;
class CategoryPDF;
class VerticalTransiogramModel;
class CommonSimulationParameters;

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

/** The Markov Chains Random Field Simulations.
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

    /** The Tau (a model of probability integration) factor for the probabilities given by the global PDF. */
    double m_tauFactorForGlobalPDF;

    /** The Tau factor for the probabilities given by the transiogram model. */
    double m_tauFactorForTransiography;

    /** The Tau factor for the probabilities given by the secondary data. */
    double m_tauFactorForProbabilityFields;

    /** The common simulation parameters (e.g. random seed number, number of realizations, search parameters, etc.) */
    CommonSimulationParameters* m_commonSimulationParameters;

    /** Runs the algorithm.  If false is returned, the call failed.  Call getLastError() to obtain the reasons. */
    bool run();

    QString getLastError() const{ return m_lastError; }

private:

    QString m_lastError;

    bool isOKtoRun();

    bool useSecondaryData();
};

#endif // MCRFSIM_H
