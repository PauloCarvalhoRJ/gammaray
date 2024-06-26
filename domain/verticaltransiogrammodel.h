#ifndef VERTICALTRANSIOGRAMMODEL_H
#define VERTICALTRANSIOGRAMMODEL_H

#include "domain/file.h"
#include "geostats/geostatsutils.h"

typedef VariogramStructureType VTransiogramStructureType;
typedef double VTransiogramRange;
typedef double VTransiogramSill;
typedef std::tuple< VTransiogramStructureType, VTransiogramRange, VTransiogramSill > VTransiogramParameters;

#define INDEX_OF_STRUCTURE_TYPE_IN_TRANSIOGRAM_PARAMETERS_TUPLE 0
#define INDEX_OF_RANGE_IN_TRANSIOGRAM_PARAMETERS_TUPLE 1
#define INDEX_OF_SILL_IN_TRANSIOGRAM_PARAMETERS_TUPLE 2

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
 *  Each tuple is: <GSLIB model code>,<vertical range>,<sill - normally between 0.0 and 1.0 as these values are probabilities>
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

    /**
     * Restores this transiogram model to its default state: empty transiogram matrix.
     */
    void clearParameters();

    /**
     * Returns the pointer to the CategoryDefinition object whose name matches the name in m_associatedCategoryDefinitionName.
     * A null pointer will be returned if the search in the project structure fails for any reason (object is not actually a
     * CategoryDefintion, metadata is missing, etc.).
     */
    CategoryDefinition* getCategoryDefinition() const;

    /** Returns the probability of the transition from one facies to another at a given separation h.
     * Make sure all transiography information was loaded with the data loading/updating methods
     * before making queries, otherwise a zero probability will be returned for any facies code not found.
     * NOTICE: Not all facies in the CatgeoryDefinition are necessarily present in the transiogram matrix.
     *         Missing categories have zero transition probability.
     */
    double getTransitionProbability( uint fromFaciesCode, uint toFaciesCode, double h ) const;

    /** Returns the number of rows/columns of transiograms in this models.
     * The number is the same because the transiogram model is necessarily made of
     * a square matrix of transiograms.
     */
    uint getRowOrColCount() const;

    /** Returns the color of the category in the index-th facies of row/col. */
    QColor getColorOfCategory( uint index ) const;

    /** Returns the name of the category in the index-th facies of row/col. */
    QString getNameOfCategory( uint index ) const;

    /** Returns the code the category in the index-th facies of row/col. */
    int getCodeOfCategory( uint index ) const;

    /** Returns the longest range of all transiograms in the model. */
    double getLongestRange() const;

    /** Returns the range for the transiogram curve in the given row and column of the model. */
    double getRange( uint iRow, uint iCol );

    /** Sets the range for the transiogram curve in the given row and column of the model. */
    void setRange( uint iRow, uint iCol, double range );

    /** Returns the sill for the transiogram curve in the given row and column of the model. */
    double getSill( uint iRow, uint iCol );

    /** Sets the sill for the transiogram curve in the given row and column of the model. */
    void setSill( uint iRow, uint iCol, double sill );

    /** Returns whether this VTM is compatible with some other VTM.  Two VTMs are said compatible if:
     * a) Both refer the same CategoryDefinion object.
     * b) The number of categories in their matrices are the same.
     * c) The categories in their matrices are the same.
     * d) The order of the categories in their matrices are the same.
     * This means that both VTMs can be used to model the same stratigraphic sequence.
     */
    bool isCompatibleWith( const VerticalTransiogramModel* otherVTM ) const;

    /** Returns the row/col index of a category given its name.
     * The index is the same for both the rows (head category) and the columns (tail category).
     * If the category name does not exist, it returns -1.
     * This method calls the getFaciesIndex() method of private API.
     * NOTE: this is not necessarily the same index of the category in the refered CategoryDefinition object.
     */
    int getCategoryMatrixIndex( const QString categoryName ) const;

    /**
     * Copy member data from otherVTM such that this VTM represents the same
     * transiogram model.
     * @attention No other data (e.g. members inherited from base classes) are copied, so this
     * method shouldn't be regarded as an equivalent of a copy constructor or deep copy.
     */
    void makeAsSameModel( const VerticalTransiogramModel& otherVTM );

    /**
     * Re-scale the sill values of the transiograms along each row so they sum up 1.0.
     * Ensuring this for transiograms modeling Markovian sequences is important.
     * @note This does not ensure a total probability of 1.0 at all separations (h).
     *       Depending on the combination of range values, sums greater or lesser than 1.0 may
     *       occur for h's shorter than the greatest transiogram range.
     */
    void unitizeRowwiseSills();

    // ProjectComponent interface
public:
    virtual QIcon getIcon();
    virtual QString getTypeName() const;
    virtual void save(QTextStream *txt_stream);

    // File interface
public:
    virtual bool canHaveMetaData();
    virtual QString getFileType() const;
    virtual void updateMetaDataFile();
    virtual void writeToFS();
    virtual void readFromFS();
    virtual void clearLoadedContents();
    virtual bool isDataFile();
    virtual bool isDistribution();
    virtual void deleteFromFS();
    virtual File* duplicatePhysicalFiles( const QString new_file_name );

private:
    ///--------------data read from metadata file------------
    QString m_associatedCategoryDefinitionName;
    ///------------------data read from file-----------------
    std::vector<QString> m_faciesNames;
    //outer vector: each line; inner vector: each transiogram model (columns)
    std::vector< std::vector < VTransiogramParameters > > m_verticalTransiogramsMatrix;

    /** Returns the row/col index of a facies given its name.
     * The index is the same for both the rows (head facies) and the columns (tail facies).
     * If the facies name does not exist, it returns -1.
     * NOTE: this is not necessarily the same index of the category in the refered CategoryDefinition object.
     */
    int getFaciesIndex( const QString faciesName ) const;

    /**
     * Returns the number of facies in this VTM.
     * NOTE: this is not necessarily the same number of categories in the refered CategoryDefinition object.
     */
    int getFaciesCount( ) const;

    /** Makes room for a new facies with default transiogram parameters in the matrix.
     * The client code must fill them accordingly.
     * Default parameters: spheric strcuture, zero range, zero sill.
     */
    void addFacies(QString faciesName);

    /** Internal facies code-to-index for fast index resolution. */
    std::map< uint, uint > m_faciesCodeToIndex;
    void updateInternalFaciesCodeToIndexMap();
};

#endif // VERTICALTRANSIOGRAMMODEL_H
