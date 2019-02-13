#ifndef FACIESTRANSITIONMATRIX_H
#define FACIESTRANSITIONMATRIX_H

#include "domain/file.h"
#include "spectral/spectral.h"

class CategoryDefinition;

/**
 * A facies transition matrix is a file containing counts (normally) of transitions from a facies in a row
 * to a facies in a column.  Some applications require that the counts be converted to probabilities
 * or proportions between 0.0 and 1.0.
 *
 * The file is a tab-separated text file like the example below (names are the name property of
 * the refered CategoryDefinition object whose name is in m_associatedCategoryDefinitionName member):
 *
 *     LMT	ESF	CRT	BRC	CAR	CRD	DOL	CLH	SLX	TUF	TRV
 * LMT	0	40	30	9	36	2	4	0	1	0	0
 * ESF	48	0	13	1	165	4	4	0	0	0	0
 * CRT	24	24	0	2	91	15	2	1	2	2	2
 * BRC	9	4	4	0	3	0	0	1	0	0	0
 * CAR	33	160	90	7	0	26	7	1	6	1	1
 * CRD	3	5	20	1	19	0	0	0	0	0	0
 * DOL	5	0	3	1	8	0	0	0	0	0	0
 * CLH	0	1	2	0	0	0	0	0	0	0	0
 * SLX	0	2	4	0	4	0	0	0	0	0	0
 * TUF	0	0	3	0	0	0	0	0	0	0	0
 * TRV	0	0	0	0	2	1	0	0	0	0	0
 *
 */
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
    void setInfo( QString associatedCategoryDefinitionName );

    /** Sets point set metadata from the accompaining .md file, if it exists.
     * Nothing happens if the metadata file does not exist.  If it exists, it calls
     * #setInfo() with the metadata read from the .md file.
     */
    void setInfoFromMetadataFile();

    /**
     * This method populates m_columnHeadersFaciesNames and m_columnHeadersFaciesNames with
     * the facies names of the category definition refered by m_associatedCategoryDefinitionName.
     * It also populates m_transitionProbabilities with zeroes.
     * This method is useful when building a FTM from scratch instead of reading from a file.
     * You must set the category definition name with a call to setInfo() or to setInfoFromMetadataFile()
     * prior to calling this method otherwise an error will ensue.
     */
    void initialize();

    /**
     * Increments by one unit the value in this matrix given two facies codes.
     * Recalling that a FTM matrix is read as the transition count/probability
     * from a facies in a line to a facies in a column.
     * An error is printed to the program's message pane if the CategoryDefinition object
     * this FTM refers to cannot be found for some reason.
     * Nothing happens if the passed code does not exist in the CategoryDefinition.
     */
    void incrementCount( int faciesCodeFrom, int faciesCodeTo );

    /**
     * Adds the values of this matrix with those of the passed FTM.
     * Nothing happens if both FTMs are not compatible for addition like mathematical matrices.
     */
    void add( const FaciesTransitionMatrix& otherFTM );

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

    int getColumnCount() const;
    int getRowCount() const;
    QString getColumnHeader( int columnIndex );
    QString getRowHeader( int rowIndex );
    double getValue( int rowIndex, int colIndex ) const;
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
    double getDownwardTransitionProbability(int toFaciesRowIndex , int fromFaciesColumnIndex);

    /**
     * Returns the transition rate value as a function of the spatial separation.
     * The value is computed as ln(TransitionProbability(h))/h, where TransitionProbability(h) is getUpwardTransitionProbability() or
     * getDownwardTransitionProbability() computed from a TFM obtained with the FaciesTransitionMatrixMaker::makeAlongTrajectory() method.
     * To make sense, the same h passed as parameter must be used to contruct the TFM.
     * The value is useful to compute experimental transiograms.
     * See Carle et al, 1998 (Conditional simulation of hydrofacies architecture: a transition probability/Markov approach.)
     * @param h The separation in length units.
     * @param upward If true, uses the transtion probability returned by getUpwardTransitionProbability(), otherwise,
     *               by getDownwardTransitionProbability().
     */
    double getTransitionRate( int faciesRowIndex, int faciesColumnIndex, double h, bool upward );

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

    /** Returns the rank of the matrix of values by the eigen decomposition method.
     * Idealluy it should be equal to the number of facies in this matrix.
     */
    int getRank();

    /** Returns a value used to test whether the sequence represented by this transition matrix
     * has a "Markovian memory" or not.  A value greater than the chi-squared distribution
     * for the same degrees of freedom ( = n^2 - 2n where n is the number of facies or the rank of
     * this matrix - see getRank() ) at 0.5% (see Util::chiSquared()) suggests
     * "Markovity" or that the facies succession has cyclicity.
     *
     * It is computed as a summation of ( getValue(i,j) - getExpectedFrequency(i,j) )^2 / getExpectedFrequency(i,j)
     * over all rows (i's) and columns (j's).
     */
    double getChiSquared();

    /** Informs whether the given column has only zeroes. */
    bool isColumnZeroed( int j ) const;

    /** Informs whether the given row has only zeroes. */
    bool isRowZeroed( int i ) const;

    /** Removes the given column from the FTM. */
    void removeColumn( int j );

    /** Removes the given row from the FTM. */
    void removeRow( int i );

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
    std::vector< std::vector < double > > m_transitionCounts;

    spectral::array toSpectralArray();
};

#endif // FACIESTRANSITIONMATRIX_H
