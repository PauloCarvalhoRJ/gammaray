#ifndef SEGMENTSET_H
#define SEGMENTSET_H

#include "domain/pointset.h"

class SegmentSet : public PointSet
{
public:
    SegmentSet( QString path );

    /** Sets segment set metadata.  It also populates the file's attribute collection. */
    void setInfo( int x_intial_index, int y_intial_index, int z_intial_index,
                  int x_final_index,  int y_final_index,  int z_final_index,
                  const QString no_data_value,
                  const QMap<uint, uint> &wgt_var_pairs,
                  const QMap<uint, QPair<uint,QString> > &nvar_var_trn_triads,
                  const QList<QPair<uint, QString> > &categorical_attributes );

    /** Calls the full setInfo() passing an empty
     * collection of variable-weight index pairs (the fourth parameter) and an empty collection
     * of variable-normal variable-transform table triads (the fifth parameter).
     */
    void setInfo( int x_intial_index, int y_intial_index, int z_intial_index,
                  int x_final_index,  int y_final_index,  int z_final_index,
                  const QString no_data_value );

    /** Sets segment set metadata from the accompaining .md file, if it exists.
     Nothing happens if the metadata file does not exist.  If it exists, it calls
     #setInfo() with the metadata read from the .md file.*/
    void setInfoFromMetadataFile();

    /**
     * Sets segment set metadata from the passed segment set. This is useful to make
     * duplicates of or to extend existing segment sets.
     */
    void setInfoFromAnotherSegmentSet( SegmentSet* otherSS );

    /** The inherited getXindex(), getYindex() and getZindex() from PointSet are the
     * coordinates of the initial segment. First index is 1 (GEO-EAS indexes).
     */
    int getXFinalIndex() const;
    int getYFinalIndex() const;
    int getZFinalIndex() const;

    /** Returns the length of the iRecord-th segment. */
    double getSegmentLenght( int iRecord );

    /** Does the same as getSegmentLenght(), but is const, which requires a prior call to DataFile::readFromFS()." */
    double getSegmentLenghtConst( int iRecord ) const;

    /** Returns the distance between the end of the iRecord-th segment
     * to the beginning of the (iRecord+1)-th segment.
     * If the iRecord-th segment is the last, this method returns zero.
     */
    double getDistanceToNextSegment( int iRecord );

    /** Does the same as getDistanceToNextSegment() but with constness, that is,
     * it does not load data from the file automatically.  An explicit call
     * to readFromFS() is necessary prior to calling this method.
     */
    double getDistanceToNextSegmentConst( int iRecord ) const;

    /**
     * Adds a new variable containing the lengths of the segments.  The values can be useful
     * for debiasing, for instance.
     */
    void computeSegmentLenghts( QString variable_name );

    /**
     * Returns (via output parameters) the bounding box of a segment given the corresponding data line index.
     */
    void getBoundingBox( uint dataLineIndex,
                         double& minX, double& minY, double& minZ,
                         double& maxX, double& maxY, double& maxZ ) const;

    /**
     * Returns wheter the given column index corresponds to one of the coordinates (x, y or z / initial or final).
     * First index is 0.
     */
    virtual bool isCoordinate( uint column ) const;

    /**
     * Creates a new PointSet object containing the data of this SegmentSet.
     * The coordinates of the point set are that of the mid points of the segments.
     * The function also creates a new physical data file matching the newly created PointSet
     * in the project's directory using the passed name as file name.
     */
    PointSet* toPointSetMidPoints(const QString &psName) const;

    /**
     * Creates a new PointSet object containing the data of this SegmentSet.
     * The coordinates of the point set are that of the segments regularely sampled.
     * The function also creates a new physical data file matching the newly created PointSet
     * in the project's directory using the passed name as file name.
     * @param step Value in length units that dictates the sampling ratio, so the lower the value
     *             the more dense is the resulting point set.
     */
    PointSet* toPointSetRegularlySpaced(const QString &psName, double step) const;

    /**
     * Creates a new segment set object similar to this one but with the data filtered
     * by the passed criteria.  To filter by a category id, just pass its code as both criteria.
     * The new point set is returned without a physical file path and not attached to the project.
     * It is up to the client code to set the path and, if needed, attach it to the project.
     */
    SegmentSet* createSegmentSetByFiltering( uint column, double vMin, double vMax );

    /**
     * Returns the |Zfinal-Zinitial| of the iRecord-th segment.
     * NOTE: make sure a prior call to DataFile::readFromFS() was made to load segment data.
     */
    double getSegmentHeight( int iRecord ) const;

    /** Returns via output parameters the x,y,z coordinate of the head vertex of the first segment. */
    void getHeadLocation( double& x, double& y, double& z ) const;

    /** Returns via output parameters the x,y,z coordinate of the tail vertex of the last segment. */
    void getTailLocation( double& x, double& y, double& z ) const;

    // ProjectComponent interface
public:
    virtual QIcon getIcon();
    virtual QString getTypeName() const;
    virtual void save( QTextStream *txt_stream );
    virtual View3DViewData build3DViewObjects( View3DWidget *widget3D );

    // ICalcPropertyCollection interface
public:
    /** NOTE: this returns the middle point of the segment. */
    virtual void getSpatialAndTopologicalCoordinates( int iRecord, double &x, double &y, double &z, int &i, int &j, int &k );

    // File interface
public:
    virtual QString getFileType() const;
    virtual void updateMetaDataFile();
    virtual File* duplicatePhysicalFiles( const QString new_file_name );

    // DataFile interface
public:
    /** NOTE: this returns the middle point of the segment. */
    virtual double getDataSpatialLocation( uint line, CartesianCoord whichCoord );
    virtual void   getDataSpatialLocation( uint line, double& x, double& y, double& z );
    /** NOTE: override the default counting-only behavior of DataFile::getProportion(). */
    virtual double getProportion(int variableIndex, double value0, double value1 );
    virtual bool getCenter( double& x, double& y, double& z ) const;

    // PointSet interface
public:
    virtual void setInfoFromOtherPointSet( PointSet* otherPS );
    virtual void deleteVariable(uint columnToDelete) override;

protected:
    int _x_final_field_index; //index start at 1. Zero means not set.
    int _y_final_field_index;
    int _z_final_field_index;
};

#endif // SEGMENTSET_H
