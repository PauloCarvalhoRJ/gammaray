#include "thresholdcdf.h"
#include "util.h"

ThresholdCDF::ThresholdCDF(QString path) : DoublesPairs( path )
{
}

QIcon ThresholdCDF::getIcon()
{
    if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
        return QIcon(":icons/thrcdf16");
    else
        return QIcon(":icons32/thrcdf32");
}

void ThresholdCDF::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
}

File *ThresholdCDF::duplicatePhysicalFiles(const QString new_file_name)
{
    QString duplicateFilePath = duplicateDataAndMetaDataFiles( new_file_name );
    return new ThresholdCDF( duplicateFilePath );
}
