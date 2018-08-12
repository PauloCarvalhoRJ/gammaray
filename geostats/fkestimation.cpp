#include "fkestimation.h"
#include "geostats/searchstrategy.h"

FKEstimation::FKEstimation() :
    m_searchStrategy( nullptr ),
    m_variogramModel( nullptr ),
    m_meanSK( 0.0 ),
    m_ktype( KrigingType::OK )
{
}

void FKEstimation::setSearchStrategy(const SearchStrategy *searchStrategy)
{
    m_searchStrategy = searchStrategy;
}

void FKEstimation::setVariogramModel(const VariogramModel *variogramModel)
{
    m_variogramModel = variogramModel;
}

void FKEstimation::setMeanForSimpleKriging(double meanSK)
{
    m_meanSK = meanSK;
}

void FKEstimation::setKrigingType(KrigingType ktype)
{
    m_ktype = ktype;
}

std::vector<double> FKEstimation::run()
{

}
