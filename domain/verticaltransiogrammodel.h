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

    /**
     * @param associatedCategoryDefinition Pass empty string to unset the value.
     */
    void setInfo( QString associatedCategoryDefinitionName );

    /** Sets metadata from the accompaining .md file, if it exists.
     * Nothing happens if the metadata file does not exist.  If it exists, it calls
     * #setInfo() with the metadata read from the .md file.
     */
    void setInfoFromMetadataFile();

    /**
     * Adds the transiographic parameters for a given a pair of facies by their names.
     * For auto-transiograms both facies are the same.
     */
    void addParameters( QString headFacies, QString tailFacies, VTransiogramParameters verticalTransiogramParameters );

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
    std::vector<QString> m_faciesNames;
    //outer vector: each line; inner vector: each transiogram model (columns)
    std::vector< std::vector < VTransiogramParameters > > m_verticalTransiogramsMatrix;

    /** Makes room for a new facies with default transiogram parameters in the matrix.
     * The client code must fill them accordingly.
     * Default parameters: spheric strcuture, zero range, zero sill.
     */
    void addFacies(QString faciesName);
};

#endif // VERTICALTRANSIOGRAMMODEL_H
