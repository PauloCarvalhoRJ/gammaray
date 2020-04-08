#ifndef MCMCDATAIMPUTATION_H
#define MCMCDATAIMPUTATION_H

#include <map>
#include <vector>
#include <QString>

class Attribute;
class SegmentSet;
class FaciesTransitionMatrix;
class UnivariateDistribution;
class CategoryPDF;

/** The type of the Facies Transition Matrix passed as the m_FTM parameter. */
enum class FTMType : int {
    FREQUENCIES,    // FTM has transition counts in its cells.
    PROBABILITIES   // FTM has transition probabilities in its cells.
};

/** The direction used to process the facies sequence for the Embedded Markov Chain simulation. */
enum class SequenceDirection : int {
    FROM_MINUS_Z_TO_PLUS_Z    // From bottom up.
};

/** Performs data imputation on data sets with an implementation of the Embedded Markov Chains-Monte
 * Carlo algorithm (MCMC). The word "embedded" means that the data sould not be regularized in deposition
 * time, so auto-transition counts are expected to be zero (the main diagonal in the Facies Transition Matrix
 * is all-zeros). This implementation uses the Mersenne Twister pseudo-random generator of 32-bit numbers with
 * a state size of 19937 bits implemented as C++ STL's std::mt19937 class to generate its Monte Carlo draw as
 * well as facies sequence draw from a Facies Transition Matrix.
 *
 * REF: Coal modeling using Markov Chain and Monte Carlo simulation: Analysis of microlithotype and lithotype
 *      succession.
 *      Dindarloo et al, Sed Geol (2015), 329: 1-11
 *      DOI: 10.1016/j.sedgeo.2015.08.005
 */
class MCMCDataImputation
{
public:
    MCMCDataImputation();

    /**
     * \defgroup MCMCimputationParameters The simulation parameters.
     */
    /*@{*/
    /** The categorical attribute to impute. */
    Attribute* m_atVariable;
    /** The data set to imputed. */
    SegmentSet* m_dataSet;
    /** The Facies Transition Matrix for the Markov Chain part of the simulation. */
    FaciesTransitionMatrix* m_FTM;
    /** The type of FTM as declared by the user. */
    FTMType m_FTMtype;
    /** The thickness distributions (map's values) of each facies (map's index) for the Monte Carlo part of the simulation. */
    std::map< int, UnivariateDistribution* > m_distributions;
    /** The seed for the random number generator. */
    int m_seed;
    /** The attribute to group the data by (e.g. drill hole id).
     * Set to nullptr so the entire data set will be treated as a single sequence.
     */
    Attribute* m_atVariableGroupBy;
    /** The flag used to set the direction used to compute the facies sequence for the
     * Markov Chain part of the simulation.
     */
    SequenceDirection m_sequenceDirection;
    /** The optional PDF to be used to draw a facies for the uninformed locations
     * that do not have a previous informed facies. An error will be thrown if such ininformed
     * locations exist and the user does not provide a PDF.
     */
    CategoryPDF* m_pdfForImputationWithPreviousUnavailable;
    /** The Facies Transition Matrix use to forbid transitions. Only transitions with count/probability - m_enforceThreshold
     * greater than zero will be allowed in the realizations.  Set nullptr to allow any transition.
     */
    FaciesTransitionMatrix* m_enforceFTM;
    /** @see m_enforceFTM */
    double m_enforceThreshold;
    /*@}*/

    /** Runs the algorithm.  If false is returned, the simulation failed.  Call getLastError() to get the reasons
     * for failure.
     * NOTE: the program appends an additional binary field so the user can distinguish between imputed data (1)
     *       and original data (0).
     */
    bool run();

    /** Returns a text explaining the cause of the last failure during the simulation. */
    QString getLastError() const{ return m_lastError; }

    /** Returns the imputed data frames (all realizations). An additional binary variable
     *  is appended to each record: 1: record was imputed; 0: record is original data.
     */
    std::vector< std::vector< std::vector<double> > > getImputedDataFrames(){ return m_imputedData; }

    /** Sets the number of realizations (the number of elements in the outermost vector of m_imputedData).
     * Any previously simulated data will be lost.
     */
    void setNumberOfRealizations(uint numberOfRealizations );

    uint getNumberOfRealizations(){ return m_imputedData.size(); }

private:

    /** The description of the cause of the last failure during simulation. */
    QString m_lastError;

    /** The imputed data frames (all realizations). */
    std::vector< std::vector< std::vector<double> > > m_imputedData;

    /** Returns whether the simulation parameters are valid and consistent. */
    bool isOKtoRun();
};

#endif // MCMCDATAIMPUTATION_H
