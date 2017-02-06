#include "thresholdcdf.h"

ThresholdCDF::ThresholdCDF(QString path) : DoublesPairs( path )
{
}

void ThresholdCDF::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
}
