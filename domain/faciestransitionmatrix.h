#ifndef FACIESTRANSITIONMATRIX_H
#define FACIESTRANSITIONMATRIX_H

#include "domain/file.h"
#include "spectral/spectral.h"

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

    /** Sums all the values in the matrix and returns it.
     * This value makes sense if this matrix is storing facies
     * transitions as counts instead of probabilities.
     * Do not forget to load the matrix previously with readFromFS(), otherwise
     * zero will be returned.
     */
    double getTotal();

    /**
     * Sums all values in a row.  First row has index 0.
     * Do not forget to load the matrix previously with readFromFS(), otherwise
     * zero will be returned.
     */
    double getSumOfRow( int rowIndex );

    /**
     * Shortcut to getTotal() - getSumOfRow(rowIndex).
     */
    double getTotalMinusSumOfRow( int rowIndex );

    /**
     * Sums all values in a column.  First column has index 0.
     * Do not forget to load the matrix previously with readFromFS(), otherwise
     * zero will be returned.
     */
    double getSumOfColumn( int columnIndex );

    int getColumnCount();
    int getRowCount();
    QString getColumnHeader( int columnIndex );
    QString getRowHeader( int rowIndex );
    double getValue( int rowIndex, int colIndex );
    double getValueMax();

    /** Returns the color assigned to the category of the given column.
     * A default color is returned if no category definition is associated
     * or the category name is not found.
     */
    QColor getColorOfCategoryInColumnHeader( int columnIndex );

    /** Returns the color assigned to the category of the given row.
     * A default color is returned if no category definition is associated
     * or the category name is not found.
     */
    QColor getColorOfCategoryInRowHeader( int rowIndex );

    /**
     * Returns the upward transition probability from one facies to another.  This value makes sense
     * for transition values stored as counts (count matrix).
     * The value is computed as getValue( fromFaciesRowIndex, toFaciesColIndex ) / getSumOfRow( fromFaciesRowIndex )
     */
    double getUpwardTransitionProbability( int fromFaciesRowIndex, int toFaciesColIndex );

    /**
     * Returns the downward transition probability from one facies to another.  This value makes sense
     * for transition values stored as counts (count matrix).
     * The value is computed as getValue( fromFaciesColumnIndex, toFaciesRowIndex ) / getSumOfColumn( fromFaciesColumnIndex )
     */
    double getDownwardTransitionProbability( int fromFaciesColumnIndex, int toFaciesRowIndex );

    /**
     * Returns the sum of post-depositional entropies for a given facies with respect to all other facies.
     * A value of zero with respect to one facies means that the given facies (faciesIndex) is always succeeded by the former.
     * A value greater than zero means that the target facies may be overlaid by other facies.
     * A large sum means that the facies occurs more independently of adjacent facies that have been deposited after (high randomness).
     * See paper "Application of Markov Chain and Entropy Function for Cyclicity Analysis
     *             of a Lithostratigraphic Sequence - A Case History from the Kolhan Basin,
     *             Jharkhand, Eastern India" for further details.
     * @param normalize If true, the result is normalized (divided by the max. entropy value).
     */
    double getPostDepositionalEntropy( int faciesIndex, bool normalize );

    /**
     * Returns the pre-depositional entropy for a given facies.
     * A value of zero with respect to one facies means that the given facies (faciesIndex) always succeeds the former.
     * A value greater than zero means that the target facies may overlay other facies.
     * A large sum means that the facies occurs independently of adjacent facies that have been deposited before (high randomness).
     * See paper "Application of Markov Chain and Entropy Function for Cyclicity Analysis
     *             of a Lithostratigraphic Sequence - A Case History from the Kolhan Basin,
     *             Jharkhand, Eastern India" for further details.
     * @param normalize If true, the result is normalized (divided by the max. entropy value).
     */
    double getPreDepositionalEntropy( int faciesIndex, bool normalize );

    /**
     * Returns the probability of the transition from one facies to another that occur in a random manner.
     * The value is computed as getSumOfColumn( toFaciesColIndex ) / ( getTotal() - getSumOfRow( fromFaciesRowIndex ) )
     */
    double getIndependentTrail( int fromFaciesRowIndex, int toFaciesColIndex );

    /**
     * This value highlight probabilities of occurrence greater than if the sequence were random.
     * Great positive values indicate an upward correlation (less random upward sequence).
     * Great negative values indicate a downward correlation (less random downward sequence).
     * It is computed as getUpwardTransitionProbability(fromFaciesRowIndex, toFaciesColIndex) - getIndependentTrail(fromFaciesRowIndex, toFaciesColIndex)
     */
    double getDifference( int fromFaciesRowIndex, int toFaciesColIndex );
    double getMaxAbsDifference( );

    /**
     * Returns the expected number of transitions from one facies to another.
     * It is computed as getIndependentTrail(fromFaciesRowIndex, toFaciesColIndex) * getSumOfRow( fromFaciesRowIndex ).
     */
    double getExpectedFrequency( int fromFaciesRowIndex, int toFaciesColIndex );
    double getMaxExpectedFrequency();

    double getRank();

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
    //outer vector: each line; inner vector: each value (columns)
    std::vector< std::vector < double> > m_transitionProbabilities;

    spectral::array toSpectralArray();
};

#endif // FACIESTRANSITIONMATRIX_H
