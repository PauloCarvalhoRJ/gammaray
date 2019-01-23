#ifndef FACIESTRANSITIONMATRIX_H
#define FACIESTRANSITIONMATRIX_H

#include "domain/file.h"

class CategoryDefinition;

class FaciesTransitionMatrix : public File
{
public:
    /** Pass empty string as associatedCategoryDefinitionName to not set an associated CategoryDefinition. */
    FaciesTransitionMatrix(QString path, QString associatedCategoryDefinitionName = "" );

    /**
     * Returns whether the facies names in the headers of the columns and rows are found
     * in the associated CategoryDefinition object.
     */
    bool isUsable();

    /**
     * @param associatedCategoryDefinition Pass nullptr to unset the value.
     */
    void setInfo(QString associatedCategoryDefinitionName );

    /** Sets point set metadata from the accompaining .md file, if it exists.
     Nothing happens if the metadata file does not exist.  If it exists, it calls
     #setInfo() with the metadata read from the .md file.*/
    void setInfoFromMetadataFile();

    /**
     * Returns the pointer to the CategoryDefinition object whose name is
     * in m_associatedCategoryDefinitionName.  Returns nullptr it the name
     * is not set or the object with the name does not exist.
     */
    CategoryDefinition* getAssociatedCategoryDefinition();

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

protected:
    ///--------------data read from metadata file------------
    QString m_associatedCategoryDefinitionName;
    ///------------------data read from file-----------------
    std::vector<QString> m_columnHeadersFaciesNames;
    std::vector<QString> m_lineHeadersFaciesNames;
    std::vector< std::vector < double> > m_transitionProbabilities;
};

#endif // FACIESTRANSITIONMATRIX_H
