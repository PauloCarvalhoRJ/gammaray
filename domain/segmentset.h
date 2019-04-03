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
    int getXFinalIndex();
    int getYFinalIndex();
    int getZFinalIndex();

    /** Returns the length of the iRecord-th segment. */
    double getSegmentLenght( int iRecord );

    /** Returns the distance between the end of the iRecord-th segment
     * to the beginning of the (iRecord+1)-th segment.
     * If the iRecord-th segment is the last, this method returns zero.
     */
    double getDistanceToNextSegment( int iRecord );


    // ProjectComponent interface
public:
    virtual QIcon getIcon();
    virtual QString getTypeName();
    virtual void save( QTextStream *txt_stream );
    virtual View3DViewData build3DViewObjects( View3DWidget *widget3D );

    // ICalcPropertyCollection interface
public:
    /** NOTE: this returns the middle point of the segment. */
    virtual void getSpatialAndTopologicalCoordinates( int iRecord, double &x, double &y, double &z, int &i, int &j, int &k );

    // File interface
public:
    virtual QString getFileType();
    virtual void updateMetaDataFile();

    // DataFile interface
public:
    /** NOTE: this returns the middle point of the segment. */
    virtual double getDataSpatialLocation( uint line, CartesianCoord whichCoord );
    /** NOTE: override the default counting-only behavior of DataFile::getProportion(). */
    virtual double getProportion(int variableIndex, double value0, double value1 );

    // PointSet interface
public:
    virtual void setInfoFromOtherPointSet( PointSet* otherPS );

protected:
    int _x_final_field_index; //index start at 1. Zero means not set.
    int _y_final_field_index;
    int _z_final_field_index;
};

#endif // SEGMENTSET_H
