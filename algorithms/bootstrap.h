#ifndef BOOTSTRAP_H
#define BOOTSTRAP_H
#include <random>

class IAlgorithmDataSource;

/*! The resampling scheme for bootstrapping. */
enum class ResamplingType : unsigned int{
    CASE /*! The simplest type.  Outputs are created by taking values from the input by Monte Carlo method. */
    //BAYESIAN /*! Outputs are created by randomly reweighting [0, 1] the input. */
    //SMOOTH /*! Outputs are created by adding a small random noise with zero mean to the input. */
    //PARAMETRIC /*! Outputs are created by radomly taking values from a parametric model fitted to the input. */
    //RESIDUALS  /*! Outputs are created such that they conserve the information of the input. */
    //GAUSSIAN /*! Outputs are created such that they conserve the temporal/spatial correlation of the input. */
    //WILD /*! Outputs are created such that they conserve the heteroscedasticity (variance is proportional to values) observed in the input.  */
    //BLOCK /*! Outputs are created such that they conserve temporal/spatial correlation or feature space correlation (clusters) of the input. */
};

/** The Bootstrap algorithm creates a new sample set from an existing one by randomly producing values
 * based on the input samples.  The new sample set may have statistics (e.g. mean) different from the
 * original samples, thus distributions of the statistics measures can be taken, which allows
 * inference of the unaccessible population.  The resulting sample set often is very different from the
 * original data, but has similar distribution and variability.  The output has the same number of samples
 * of the input.  Other characteristics of the input (e.g. correlations) can be preserved depending on the
 * type of resampling of choice.
 */
class Bootstrap
{
public:
    Bootstrap( const IAlgorithmDataSource& input,
               ResamplingType resType,
               long randomNumberGeneratorSeed );

    void resample( IAlgorithmDataSource& result );

protected:
    const IAlgorithmDataSource& m_input;
    ResamplingType m_resType;
    long m_seed;
    std::mt19937 m_randomNumberGenerator;
};

#endif // BOOTSTRAP_H
