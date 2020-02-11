#ifndef MCMCDATAIMPUTATION_H
#define MCMCDATAIMPUTATION_H

#include <vector>
#include <QString>

class Attribute;
class SegmentSet;
class FaciesTransitionMatrix;
class UnivariateDistribution;

/** The type of the Facies Transition Matrix passed as the m_FTM parameter. */
enum class FTMType : int {
    FREQUENCIES,    // FTM has transition counts in its cells.
    PROBABILITIES   // FTM has transition probabilities in its cells.
};


/** Performs data imputation on data sets with an implementation of the Embedded Markov Chains-Monte Carlo algorithm (MCMC).
 * The word "embedded" means that the data sould not be regularized in deposition time, so auto-transition counts are expected
 * to be zero (the main diagonal in the Facies Transition Matrix is all-zeros). This implementation uses the Mersenne Twister
 * pseudo-random generator of 32-bit numbers with a state size of 19937 bits implemented as C++ STL's std::mt19937 class to
 * generate its Monte Carlo draw as well as facies sequence draw from a Facies Transition Matrix.
 *
 * REF: Coal modeling using Markov Chain and Monte Carlo simulation: Analysis of microlithotype and lithotype succession.
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
    /** The thickness distributions of each facies for the Monte Carlo part of the simulation. */
    std::vector< UnivariateDistribution* > m_distributions;
    /** The seed for the random number generator. */
    int m_seed;
    /*@}*/

    /** Runs the algorithm.  If false is returned, the simulation failed.  Call getLastError() to obtain the reasons. */
    bool run();

    /** Returns a text explaining the cause of the last failure during the simulation. */
    QString getLastError() const{ return m_lastError; }

private:

    /** The description of the cause of the last failure during simulation. */
    QString m_lastError;

    /** Returns whether the simulation parameters are valid and consistent. */
    bool isOKtoRun();
};

#endif // MCMCDATAIMPUTATION_H
