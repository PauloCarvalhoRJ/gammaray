#include "univariatedistribution.h"


UnivariateDistribution::UnivariateDistribution(const QString path) : Distribution( path )
{
}

QIcon UnivariateDistribution::getIcon()
{
    return QIcon(":icons/unidist16");
}


