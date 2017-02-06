#include "bivariatedistribution.h"

BivariateDistribution::BivariateDistribution(const QString path) : Distribution( path )
{
}

QIcon BivariateDistribution::getIcon()
{
    return QIcon(":icons/bidist16");
}

