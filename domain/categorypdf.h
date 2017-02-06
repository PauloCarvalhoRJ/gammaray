#ifndef CATEGORYPDF_H
#define CATEGORYPDF_H

#include "valuepairs.h"

typedef ValuePairs<int,double> IntDoublePairs;

class CategoryPDF : public IntDoublePairs
{
public:
    CategoryPDF( QString path );

    // ProjectComponent interface
public:
    QIcon getIcon(){return QIcon(":icons/catpdf16");}
    void save(QTextStream *txt_stream);

    // File interface
public:
    bool canHaveMetaData(){ return false; }
    QString getFileType(){ return "CATEGORYPDF"; }
    void updateMetaDataFile(){}
    virtual bool isEditable(){ return true; }
};

#endif // CATEGORYPDF_H
