#ifndef TAUMODEL_H
#define TAUMODEL_H

#include <vector>
#include <memory>

/** This is an implementation of the Tau Model according to Krishnan (2004).  The Tau Model
 *  is a type of probability integration model.  In other words, it is a way to integrate
 *  information from different sources.  A Tau Factor of 0.0 means that the corresponding source
 *  is ignored.  All Tau Factors set to 1.0 means that all sources are equally informative.
 *  The greater thet Tau Factor, more influent is the corresponding source.
 *
 *   Formulation for 2 sources:
 *                 T          T
 *    x     /  b  \ b  /  c  \ c
 *     k    |   k |    |   k |
 *   ____ = | ____|    | ____|
 *    a     |  a  |    |  a  |
 *     k    \   k /    \   k /
 *
 * Where
 *             1-pa        pa = marginal probability of k-th category
 *    a   =        k         k
 *     k       -----
 *              pa         pb = probability of k-th category given by 1st source (e.g. transiogram)
 *                k          k
 *
 *             1-pb        pc = probability of k-th category given by 2nd source (e.g. secondary data)
 *    b   =        k         k
 *     k       -----
 *              pb         T  = Tau Factor for the 1st source.
 *                k         b
 *
 *             1-pc        T  = Tau Factor for the 2nd source.
 *    c   =        k        c
 *     k       -----
 *              pc
 *                k
 *
 * REF - Krishnan, S. 2004. Combining individual data information: A review and the tau model.
 *       Stanford Center for Reservoir Forecasting Annual Report.
 */
class TauModel
{
public:
    /**
     * @param nCategories Sets the number of categories.
     * @param nSources Sets the number of probability sources in addition to
     *                 the marginal probabilities.  Lowest possible value is 1.
     */
    TauModel( unsigned int nCategories, unsigned int nSources );

    void setMarginalProbability( unsigned int categoryIndex,
                                 double marginalProbability );

    void setProbabilityFromSource( unsigned int categoryIndex,
                                   unsigned int sourceIndex,
                                   double probabilityFromSource );

    void setTauFactor( unsigned int sourceIndex,
                       double tauFactor );

    double getFinalProbability( unsigned int categoryIndex ) const;

private:
    unsigned int m_nCategories;
    std::vector<double> m_probsMarginal;
    std::vector< std::vector<double> > m_probsOtherSources;
    std::vector<double> m_TauFactors;
};

typedef std::shared_ptr< TauModel > TauModelPtr;

#endif // TAUMODEL_H
