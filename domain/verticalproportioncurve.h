#ifndef VERTICALPROPORTIONCURVE_H
#define VERTICALPROPORTIONCURVE_H

#include "domain/datafile.h"
#include "domain/categorydefinition.h"

/** An entry in the Vertical Proportion Curve */
struct VPCEntry{
    VPCEntry( double pRelativeDepth, const CategoryDefinition* cd ) :
        relativeDepth( pRelativeDepth ), proportions( cd->getCategoryCount() ){}
    /** The relave depth varying between 0.0 (base) and 1.0 (top). */
    double relativeDepth;
    /** The proportions of facies varying between 0.0 (0%) and 1.0 (100%) and summing up 1.0. */
    std::vector<double> proportions;
};

/** Reasons for incompatibility between two VPCs. */
enum class VPCIncompatibilityReason : uint {
    NULL_CATEGORY_DEFINITION,
    DIFFERENT_CATEGORY_DEFINITIONS,
    DIFFERENT_ENTRY_COUNTS,
    DIFFERENT_PROPORTION_COUNTS,
    DIFFERENT_RELATIVE_DEPTHS
};

/**
 * The VerticalProportionCurve class models a Vertical Proportion Curve, a series of values defining
 * the expected proportion of categories (e.g. lithofacies) along the vertical.  The vertical scale of
 * the curve is relative, varying from 1.0 at top and 0.0 at the base.  The absolute depth at which a
 * certain proportion occurs should be computed on the fly with respect to some reference such as top
 * and base horizons or max/min depth of point sets or segment sets.  The curves are represented by polygonal
 * lines between top and base. The relative depth and proportions for each facies are values between 0.0 and 1.0
 * and are stored as columns of a GEO-EAS file (similiarly to the data sets).  Each line of the GEO-EAS file
 * corresponds to a relative depth.  The column names in the resulting GEO-EAS files are generated sequentially
 * for conformance to the file format and have no meaning.
 *
 * Although this class extends DataFile, it does so to reuse the file reading/writing infrastructure
 * as VPC's are not spatial objects like wells, drillholes, horizons, etc.
 */
class VerticalProportionCurve : public DataFile
{
public:
    VerticalProportionCurve( QString path, QString associatedCategoryDefinitionName );

    /** Adds a new entry with zeros for all categories for the given relative depth (0.0==base, 1.0==top). */
    void addNewEntry( double relativeDepth );

    /** Returns the current number of entries in this VPC. */
    int getEntriesCount() const;

    /** Returns the number of proportion values in the first entry in this curve.
     * If this curve is empty (no entries), it returns zero.
     */
    int getProportionsCount() const;

    /** Sets a proportion value for a given category in one of the entries. */
    void setProportion( int entryIndex, int categoryCode, double proportion );

    /** Returns whether this VPC has proportions entries. */
    bool isEmpty() const;

    /** Sets this VPC as the mean of the VPCs passed as parameter.
     * If the curves are not compatible for some reason (e.g. different number of entries),
     * the method fails (returns false).  If there is only one curve, the curve values are copied
     * to this curve.  Nothing changes if the passed vector is empty.
     * @note This method does not change the other properties of this object.  It only
     *       changes the values in the m_entries member.
     */
    bool setAsMeanOf( const std::vector< VerticalProportionCurve >& curves );

    /** Returns whether this VPC is compatible with another VPC.  This means whether
     * Both VPCs refer to the same CategoryDefinition object, the number of entries is the same,
     * the number of proportions in each entry is the same and the relative depths (0.0==base, 1.0==top)
     * are the same.
     * @param reason Output parameter to know the reason for incompatibility.
     */
    bool isCompatibleWith( const VerticalProportionCurve& otherVPC,
                           VPCIncompatibilityReason& reason ) const;

    /** Returns the relative depth (0.0==base, 1.0==top) for the i-th entry of the curve.
     * Returns NaN (not-a-number) if the curve has no entries or the passed index is invalid.
     */
    double getRelativeDepth( uint entryIndex ) const;

    /**
     * Returns the pointer to the CategoryDefinition object whose name is
     * in m_associatedCategoryDefinitionName.  Returns nullptr it the name
     * is not set or the object with the name does not exist.
     */
    CategoryDefinition* getAssociatedCategoryDefinition() const;

    /** Returns the n-th proportion values of all entries. */
    std::vector< double > getNthProportions( uint proportionIndex ) const;

    /** Returns the n-th cumulative (that is, summung all values from the first proportion)
     * proportion values of all entries. */
    std::vector< double > getNthCumulativeProportions( uint proportionIndex ) const;

    /** Prints the values of this curve in tabular form.
     * Useful for debugging or to generate data files. */
    void print() const;

    /** Sets vertical proportion curve metadata from the accompaining .md file, if it exists.
     * Nothing happens if the metadata file does not exist.  If it exists, it calls
     * #setInfo() with the metadata read from the .md file.
     */
    void setInfoFromMetadataFile();

    /**
     * @param associatedCategoryDefinition Pass empty text to unset the value.
     */
    void setInfo( QString associatedCategoryDefinitionName );

    /** Returns the name given to the data column containing the values of all
     * entries for a proportion given its index.
     */
    QString getProportionVariableName( uint proportionIndex ) const;

    /** Returns a proportion value at the given relative depth (0.0 == base, 1.0 == top)
     * A linear interpolation is performed for relative depth values not explicitly
     * present in the entries.
     * @note To not be confused with the totally unrelated DataFile::getProportion().
     */
    double getProportionAt(uint proportionIndex, double relativeDepth) const;

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
    virtual bool isDataFile();
    virtual bool isDistribution();
    virtual void deleteFromFS();
    virtual File* duplicatePhysicalFiles( const QString new_file_name );

    //DataFile interface
public:
    bool isWeight( Attribute* at );
    Attribute* getVariableOfWeight( Attribute* weight );
    virtual void deleteVariable( uint columnToDelete );
    virtual bool isRegular();
    virtual double getDataSpatialLocation( uint line, CartesianCoord whichCoord );
    virtual void getDataSpatialLocation( uint line, double& x, double& y, double& z );
    virtual bool isTridimensional();
    virtual void writeToFS();
    virtual void readFromFS();
    virtual bool isGridded() const override {  return false; }
    virtual BoundingBox getBoundingBox( ) const override;

    // ICalcPropertyCollection interface
public:
    virtual void getSpatialAndTopologicalCoordinates( int iRecord, double& x, double& y, double& z, int& i, int& j, int& k );
    virtual double getNeighborValue( int iRecord, int iVar, int dI, int dJ, int dK );

protected:
    ///--------------data read from metadata file------------
    QString m_associatedCategoryDefinitionName;

    std::vector< VPCEntry > m_entries;
};

typedef std::shared_ptr<VerticalProportionCurve> VerticalProportionCurvePtr;

#endif // VERTICALPROPORTIONCURVE_H
