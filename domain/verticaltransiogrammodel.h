#ifndef VERTICALTRANSIOGRAMMODEL_H
#define VERTICALTRANSIOGRAMMODEL_H

#include "domain/file.h"
#include "geostats/geostatsutils.h"

typedef VariogramStructureType VTransiogramStructureType;
typedef double VTransiogramRange;
typedef double VTransiogramSill;
typedef std::tuple< VTransiogramStructureType, VTransiogramRange, VTransiogramSill > VTransiogramParameters;

/**
 * A vertical transiogram model is a file containing the comma-separated tuples of model parameters in
 * a matrix of facies.
 *
 * The file is a tab-separated text file like the example below (names are the name property of
 * the refered CategoryDefinition object whose name is in m_associatedCategoryDefinitionName member):
 *
 *  LMT	ESF	CRT
 *LMT	1,2.3,0.22	1,2.1,0.13	1,3.2,0.98
 *ESF	1,1.2,0.51	1,0.8,0.45	1,1.7,0.33
 *CRT	1,0.7,0.74	1,1.9,0.67	1,2.6,0.19
 *
 *  Each tuple is: <GSLIB model code>,<vertical range>,<sill - normally between 0.0 and 1.0>
 */

class VerticalTransiogramModel : public File
{
public:
    VerticalTransiogramModel(QString path,
                             QString associatedCategoryDefinitionName);

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

private:
    ///--------------data read from metadata file------------
    QString m_associatedCategoryDefinitionName;
    ///------------------data read from file-----------------
    std::vector<QString> m_columnHeadersFaciesNames;
    std::vector<QString> m_lineHeadersFaciesNames;
    //outer vector: each line; inner vector: each transiogram model (columns)
    std::vector< std::vector < VTransiogramParameters > > m_verticalTransiogramsMatrix;
};

#endif // VERTICALTRANSIOGRAMMODEL_H
