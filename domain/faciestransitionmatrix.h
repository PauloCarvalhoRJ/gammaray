#ifndef FACIESTRANSITIONMATRIX_H
#define FACIESTRANSITIONMATRIX_H

#include "domain/file.h"

class CategoryDefinition;

class FaciesTransitionMatrix : public File
{
public:
    FaciesTransitionMatrix( QString path, CategoryDefinition* associatedCategoryDefinition );

    /**
     * Returns whether the facies names in the headers of the columns and rows are found
     * in the associated CategoryDefinition object.
     */
    bool isUsable();

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

protected:
    CategoryDefinition* m_associatedCategoryDefinition;

    std::vector<QString> m_columnHeadersFaciesNames;
    std::vector<QString> m_lineHeadersFaciesNames;
    std::vector< std::vector < double> > m_transitionProbabilities;
};

#endif // FACIESTRANSITIONMATRIX_H
