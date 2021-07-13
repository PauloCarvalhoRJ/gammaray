#ifndef THRESHOLDCDF_H
#define THRESHOLDCDF_H

#include "valuepairs.h"

typedef ValuePairs<double,double> DoublesPairs;

class ThresholdCDF : public DoublesPairs
{
public:
    ThresholdCDF( QString path );

    // ProjectComponent interface
public:
    QIcon getIcon();
    void save(QTextStream *txt_stream);

    // File interface
public:
    bool canHaveMetaData(){ return false; }
    QString getFileType() const { return "THRESHOLDCDF"; }
    void updateMetaDataFile(){}
    virtual bool isEditable(){ return true; }
    bool isDataFile(){ return false; }
	bool isDistribution(){ return false; } //Although a CDF is technically a distribution, it doesn't inherit Distribution
    virtual File* duplicatePhysicalFiles( const QString new_file_name );
};

#endif // THRESHOLDCDF_H
