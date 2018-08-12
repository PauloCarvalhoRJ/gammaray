#ifndef FKESTIMATION_H
#define FKESTIMATION_H

#include "geostatsutils.h"

class SearchStrategy;
class VariogramModel;

/** This class encpsulates the factorial kriging estimation.
 */
class FKEstimation
{
public:
    FKEstimation();

    //@{
    /** Factorial Kriging parameters. */
    void setSearchStrategy( const SearchStrategy* searchStrategy );
    void setVariogramModel( const VariogramModel* variogramModel );
    void setMeanForSimpleKriging( double meanSK );
    void setKrigingType( KrigingType ktype );
    //@}

    /** Preforms the factorial kriging. Make sure all parameters have been set properly .*/
    std::vector<double> run();

private:
    const SearchStrategy *m_searchStrategy;
    const VariogramModel *m_variogramModel;
    double m_meanSK;
    KrigingType m_ktype;
};

#endif // FKESTIMATION_H
