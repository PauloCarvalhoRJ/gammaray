#include "bivariatedistribution.h"
#include "util.h"

BivariateDistribution::BivariateDistribution(const QString path) : Distribution( path )
{
}

QIcon BivariateDistribution::getIcon()
{
    if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
        return QIcon(":icons/bidist16");
    else
        return QIcon(":icons32/bidist32");
}

