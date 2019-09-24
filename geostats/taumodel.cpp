#include "taumodel.h"
#include <cmath>

TauModel::TauModel( unsigned int nCategories , unsigned int nSources ) :
    m_nCategories( nCategories ),
    m_probsMarginal( nCategories, 0.0 ),
    m_probsOtherSources( nSources, std::vector<double>( nCategories, 0.0) ),
    m_TauFactors( nSources, 1.0 )
{
}

void TauModel::setMarginalProbability(unsigned int categoryIndex,
                                      double marginalProbability)
{
    m_probsMarginal[categoryIndex] = marginalProbability;
}

void TauModel::setProbabilityFromSource(unsigned int categoryIndex,
                                        unsigned int sourceIndex,
                                        double probabilityFromSource)
{
    m_probsOtherSources[sourceIndex][categoryIndex] = probabilityFromSource;
}

void TauModel::setTauFactor(unsigned int sourceIndex,
                            double tauFactor)
{
    m_TauFactors[sourceIndex] = tauFactor;
}

double TauModel::getFinalProbability(unsigned int categoryIndex) const
{
    //get marginal ratio
    double pa = m_probsMarginal[ categoryIndex ];
    double a = ( 1.0 - pa ) / pa;

    //update ratio with the sources of information
    double x = 1.0;
    for( unsigned int sourceIndex = 0; sourceIndex < m_probsOtherSources.size(); ++sourceIndex ){
        double pb = m_probsOtherSources[sourceIndex][ categoryIndex ];
        double b = pb==0.0 ? 1000.0 : ( ( 1.0 - pb ) / pb ); //resolve to an arbitrarily large value if pb is zero (b tends to be large)
        x *= std::pow( b / a, m_TauFactors[sourceIndex] );
    }

    //solving for the final probability px (see DOxygen comment of this class for the formulation)
    x *= a;
    return 1.0 / ( 1.0 + x );
}
