#ifndef QUADRATIC3DTRENDMODELFITTING_H
#define QUADRATIC3DTRENDMODELFITTING_H

#include <stdint.h>
#include <cassert>
#include <vector>
#include <limits>
#include <cmath>

class DataFile;
class Attribute;
class QProgressDialog;

/**
 * A pointer to a progress dialog so a GSL's C-style callback function can access it
 * to display algorithm progress.
 */
static QProgressDialog* s_progressDiag_for_iteration_callback = nullptr;

namespace Quad3DTrendModelFittingAuxDefs {

    /** The number of model parameters. */
    constexpr static int N_PARAMS = 9;

    /** A shortcut to a standard NaN value. */
    constexpr static double NaN = std::numeric_limits<double>::quiet_NaN();

    /** Data structure: the nine model parameters to find. */
    struct Parameters {
        Parameters():a(NaN),b(NaN),c(NaN),d(NaN),e(NaN),f(NaN),g(NaN),h(NaN),i(NaN){} //all-NaN intialization
        Parameters(double a, double b, double c, double d, double e, double f, double g, double h, double i):
            a(a),b(b),c(c),d(d),e(e),f(f),g(g),h(h),i(i){} //valid values intialization
        double a, b, c, d, e, f, g, h, i; //the individual parameters
            //xx,xy,xz,yy,yz,zz, x, y, z  //the model terms corresponding to the parameters
        double& operator [](int index){ //this allows the set to be used as an array (l-value).
            switch(index){
            case 0: return a;  case 1: return b; case 2: return c;
            case 3: return d;  case 4: return e; case 5: return f;
            case 6: return g;  case 7: return h; case 8: return i;
            default: assert( false && "Quadratic3DTrendModelFitting::Parameters: index out of bounds (must be between 0 and 8).");
            }
        }
        double operator [](int index) const { //this allows the set to be used as an array (r-value).
            switch(index){
            case 0: return a;  case 1: return b; case 2: return c;
            case 3: return d;  case 4: return e; case 5: return f;
            case 6: return g;  case 7: return h; case 8: return i;
            default: assert( false && "Quadratic3DTrendModelFitting::Parameters: index out of bounds (must be between 0 and 8).");
            }
        }
        bool isTrivial() const {
            return a==0.0 && b==0.0 && c==0.0 && d==0.0 && e==0.0 && f==0.0 && g==0.0 && h==0.0 && i==0.0;
        }
        void zeroOutZBearingTerms(){ c = e = f = i = 0.0; } //this is mostly used with 2D datasets (z-less data)
    };

    /** Data structure: the parameters domain for the optimization methods (boundary conditions). */
    struct ParametersDomain {
        Parameters min;
        Parameters max;
        bool isNearBoundary( const Parameters& parametersToTest, double threshold ) const {
            for( int i = 0; i < N_PARAMS; i++ ){
                if( std::abs(parametersToTest[i] - min[i]) <= threshold )
                    return true;
                else if( std::abs(parametersToTest[i] - max[i]) <= threshold )
                    return true;
            }
            return false;
        }
        void centerAt( const Parameters& targetParameters, double domainSize ){
            double halfSize = domainSize/2;
            for( int i = 0; i < N_PARAMS; i++ ){
                min[i] = targetParameters[i] - halfSize;
                max[i] = targetParameters[i] + halfSize;
            }
        }
    };
}

/**
 * The Quadratic3DTrendModelFitting class comprises the functionalities to fit a 3D quadratic trend model
 * to a data set.  The trend model has nine terms in the form:
 *              f(x,y,z) = a*x*x + b*x*y + c*x*z + d*y*y + e*y*z + f*z*z + g*x + h*y + i*z
 *              where: a...i are the model parameters.
 * This form follows the one expected in the configuration of the kt3d GSLib program for kriging with
 * a trend model.  The model predicts the value of a continuous variable given a location in space.
 * The fitting process searchs for a set of a...i parameters such that the differences between f(x,y,z)
 * and the observed values at data locations are minimized.
 */
class Quadratic3DTrendModelFitting
{
public:

    /** Constructor. */
    Quadratic3DTrendModelFitting(DataFile* dataFile, Attribute* attribute);



    /** The objective function for an optimization process to find a minimum (ideally the global one).
     * @param parameters The parameters of the model to test.
     * @return The sum of the differences between all the observed and predicted values.
     */
    double objectiveFunction (const Quad3DTrendModelFittingAuxDefs::Parameters& parameters) const;


    /** Method called by a method of optimization to initialize the
     *  parameter domain (boundary conditions) and the set of parameters.
     * All non-const method parameters are output parameters.
     * @param domain The domain boundaries to initialize as two sets of
     *               parameters: with the minima and with the maxima.
     * @param searchWindowSize Defines the size of the parameter domain.  E.g. if set to 10.00, then each of the nine coefficients
     *                         will be restricted to an initial interval between -10.00 and 10.00.
     */
    void initDomain( Quad3DTrendModelFittingAuxDefs::ParametersDomain& domain,
                     double                                            searchWindowSize) const;

    /** Performs the trend model fitting using a Genetic Algorithm as optimization method.
     *...................................Global Parameters....................................
     * @param nThreads Number of parallel execution threads.
     * @param seed Seed for the the random number generator.
     *...................................GA Parameters.................................
     * @param maxNumberOfGenerations number of generations (iterations).
     * @param nPopulationSize Number of individuals (solutions).  Mas be an even number.
     * @param nSelectionSize The size of the selection pool (must be < nPopulationSize and be an even number).
     * @param probabilityOfCrossOver The probability of crossover (between 0.0 and 1.0);
     * @param pointOfCrossover The parameter index from which crossover takes place (must be less than the total
     *                         number of parameters per individual).
     * @param mutationRate Mutation rate means how many paramaters are expected to change per mutation
     *                     the probability of any parameter parameter (gene) to be changed is 1/nParameters * mutationRate
     *                     thus, 1.0 means that one gene will surely be mutated per mutation on average.  Fractionary
     *                     values are possible. 0.0 means no mutation will take place.
     * @param searchWindowSize Defines the size of the parameter domain.  E.g. if set to 10.00, then each of the nine coefficients
     *                         will be restricted to an initial interval between -10.00 and 10.00.
     * @param windowWindowShiftThreshold If any of the coefficients of the optimal model lies close to the boundaries of the domain
     *                                   then the algorithm defines a new parameter domain centered on it and resumes search.  This
     *                                   value controls how close a parameter must be to a domain boundary to enable a new search.
     * @returns The set of parameters of the fit trend model.  An all-NaN set of parameters is returned if something goes
     *          wrong during processing (error messages will be displayed).
     */
    Quad3DTrendModelFittingAuxDefs::Parameters processWithGenetic(
                            uint16_t nThreads,
                            uint32_t seed,
                            uint16_t maxNumberOfGenerations,
                            uint16_t nPopulationSize,
                            uint16_t nSelectionSize,
                            double probabilityOfCrossOver,
                            uint8_t pointOfCrossover,
                            double mutationRate,
                            double searchWindowSize,
                            double windowWindowShiftThreshold
            ) const;

    Quad3DTrendModelFittingAuxDefs::Parameters processWithNonLinearLeastSquares() const;


private:
    Attribute* m_attribute;
    DataFile* m_dataFile;

    /** The objective function values collected during the last execution
     * of an optimization method.
     */
    static std::vector< double > s_objectiveFunctionValues;

    /**
     * Displays a dialog with a chart showing the evolution of the objective function
     * value versus iterations of the optimization method.
     */
    void showObjectiveFunctionEvolution( ) const;
};

#endif // QUADRATIC3DTRENDMODELFITTING_H
