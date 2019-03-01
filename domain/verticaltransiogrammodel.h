#ifndef VERTICALTRANSIOGRAMMODEL_H
#define VERTICALTRANSIOGRAMMODEL_H

#include "domain/file.h"

class VerticalTransiogramModel : public File
{
public:
    VerticalTransiogramModel();

    // ProjectComponent interface
public:
    virtual QIcon getIcon();
    virtual QString getTypeName();
    virtual void save(QTextStream *txt_stream);

    // File interface
public:
    virtual bool canHaveMetaData();
    virtual QString getFileType();
    virtual void updateMetaDataFile();
    virtual void writeToFS();
    virtual void readFromFS();
    virtual void clearLoadedContents();
    virtual bool isDataFile();
    virtual bool isDistribution();
    virtual void deleteFromFS();


};

#endif // VERTICALTRANSIOGRAMMODEL_H
