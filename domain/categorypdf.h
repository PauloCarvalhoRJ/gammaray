#ifndef CATEGORYPDF_H
#define CATEGORYPDF_H

#include "valuepairs.h"

typedef ValuePairs<int,double> IntDoublePairs;

class CategoryDefinition;

class CategoryPDF : public IntDoublePairs
{
public:
    CategoryPDF( CategoryDefinition *cd, QString path );
    /** Constructor with the name of a CategoryDefinition (delayed load).
      * @param categoryDefinitionName The name of a CategoryDefinition object existing in the project.
      */
    CategoryPDF( const QString categoryDefinitionName, QString path );

    /** Returns the CategoryDefinition object this object was based on.
        Returns nullptr if this object was not created from a CategoryDefinition (older versions of GammaRay)*/
    CategoryDefinition *getCategoryDefinition();

    // ProjectComponent interface
public:
    QIcon getIcon();
    void save(QTextStream *txt_stream);

    // File interface
public:
    bool canHaveMetaData(){ return false; }
    QString getFileType(){ return "CATEGORYPDF"; }
    void updateMetaDataFile(){}
    virtual bool isEditable(){ return true; }
    bool isDataFile(){ return false; }

private:
    CategoryDefinition *m_categoryDefinition;

    /** This value does not need to be updated after m_categoryDefinition pointer is set. */
    QString m_categoryDefinitionNameForDelayedLoad;

    /** Sets m_categoryDefinition by searching a CategoryDefinition with the name given by
     * m_categoryDefinitionNameForDelayedLoad.
     * Returns true if the search was successful.
     */
    bool setCategoryDefinition();
};

#endif // CATEGORYPDF_H
