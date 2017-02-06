#include "roles.h"

namespace Roles {

QString getRoleText(DistributionColumnRole role)
{
    switch( role ){
        case DistributionColumnRole::VALUE:
            return "Values";
        case DistributionColumnRole::LOGVALUE:
            return "Values (log scale)";
        case DistributionColumnRole::PVALUE:
            return "Probabilities";
        default:
            return "Unknown";
    }
}

QIcon getRoleIcon(DistributionColumnRole role)
{
    switch( role ){
        case DistributionColumnRole::VALUE:
            return QIcon(":icons/distvalue16");
        case DistributionColumnRole::LOGVALUE:
            return QIcon(":icons/distlogvalue16");
        case DistributionColumnRole::PVALUE:
            return QIcon(":icons/distprob16");
        default:
            return QIcon(":icons/distundef16");
    }
}

}

