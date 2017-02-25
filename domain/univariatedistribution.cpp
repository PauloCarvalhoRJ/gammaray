#include "univariatedistribution.h"
#include "util.h"


UnivariateDistribution::UnivariateDistribution(const QString path) : Distribution( path )
{
}

QIcon UnivariateDistribution::getIcon()
{
    if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
        return QIcon(":icons/unidist16");
    else
        return QIcon(":icons32/unidist32");
}


