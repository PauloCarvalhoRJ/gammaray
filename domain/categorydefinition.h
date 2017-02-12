#ifndef CATEGORYDEFINITION_H
#define CATEGORYDEFINITION_H

#include "triads.h"

typedef Triads<int,int,QString> IntIntQStringTriplets;

class CategoryDefinition : public IntIntQStringTriplets
{
public:
    CategoryDefinition( QString path );

    // ProjectComponent interface
public:
    QIcon getIcon(){return QIcon(":icons/catdef16");}
    void save(QTextStream *txt_stream);

    // File interface
public:
    bool canHaveMetaData(){ return false; }
    QString getFileType(){ return "CATEGORYDEFINITION"; }
    void updateMetaDataFile(){}
    virtual bool isEditable(){ return true; }
};

#endif // CATEGORYDEFINITION_H
