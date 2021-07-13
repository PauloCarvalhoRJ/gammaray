#include "bivariatedistribution.h"
#include "util.h"

BivariateDistribution::BivariateDistribution(const QString path) : Distribution( path )
{
}

File *BivariateDistribution::duplicatePhysicalFiles(const QString new_file_name)
{
    QString duplicateFilePath = duplicateDataAndMetaDataFiles( new_file_name );
    return new BivariateDistribution( duplicateFilePath );
}

QIcon BivariateDistribution::getIcon()
{
    if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
        return QIcon(":icons/bidist16");
    else
        return QIcon(":icons32/bidist32");
}

