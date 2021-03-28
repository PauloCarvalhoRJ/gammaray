#ifndef POINTSET_H
#define POINTSET_H
#include "datafile.h"
#include <QString>
#include <QMap>

class Attribute;

/**
 * @brief The PointSet class represents a point set data file.  A point set is made of scattered values
 * located in space.
 */
class PointSet : public DataFile
{
public:
    PointSet( QString path );

    /** Sets point set metadata.  It also populates the file's attribute collection. */
    void setInfo(int x_index, int y_index, int z_index, const QString no_data_value,
                  const QMap<uint, uint> &wgt_var_pairs,
                  const QMap<uint, QPair<uint,QString> > &nvar_var_trn_triads,
                  const QList<QPair<uint, QString> > &categorical_attributes );

    /** Calls setInfo(int, int, int, const QString, const QMap<uint, uint> &) passing an empty
     * collection of variable-weight index pairs (the fourth parameter) and an empty collection
     * of variable-normal variable-transform table triads (the fifth parameter).
     */
    void setInfo( int x_index, int y_index, int z_index, const QString no_data_value );

    /** Sets point set metadata from the accompaining .md file, if it exists.
     Nothing happens if the metadata file does not exist.  If it exists, it calls
     #setInfo(int,int,int,const QString) with the metadata read from the .md file.*/
    void setInfoFromMetadataFile();

    /** Sets point set metadata from the passed point set. This is useful to make
     * duplicates of or to extend existing point sets.
     */
    virtual void setInfoFromOtherPointSet( PointSet* otherPS );

    /** First index is 1 (GEO-EAS indexes) */
    int getXindex() const;
    int getYindex() const;
    int getZindex() const;

    /**
     * Returns whether the Z coordinate was defined.
     */
    bool is3D() const;

    /**
     *  Adds a new variable-weight relationship in the form given by their
     *  index in the GEO-EAS file.
     */
    void addVariableWeightRelationship( uint variableGEOEASindex, uint weightGEOEASindex );

    /**
     * Returns wheter the given column index corresponds to one of the coordinates (x, y or z).
     * First index is 0.
     */
    virtual bool isCoordinate( uint column ) const;

    /** Returns a list with pairs with weight-variable relations.
     *  The key of the map is the index of the variable which is the weight
     *  of the variable whose index is the second member of the pair.
     */
    QMap<uint, uint> getWeightsVariablesPairs() const  { return _wgt_var_pairs; }

    /**
     * Creates a new point set object similar to this one but with the data filtered
     * by the passed criteria.  To filter by a category id, just pass its code as both criteria.
     * The new point set is returned without a physical file path and not attached to the project.
     * It is up to the client code to set the path and, if needed, attach it to the project.
     */
    PointSet* createPointSetByFiltering( uint column, double vMin, double vMax );

    /**
     * Duplicates the given data row.  The new data row is appended right after the original
     * data row.  If the row index is out of range or if the data set is empty, a crash may ensue.
     * Client code is responsible for commiting the changed data to the physical file by calling writeToFS().
     * @note This method was not added to the DataFile superclass as coloning individual data
     *       rows breaks the data order of gridded data sets.
     * @note Coloning data rows potentially involves reallocating large quantities of data, so it may be slow
     *       if called too often.  Use sparingly.
     */
    void cloneDataLine( uint row );

    //DataFile interface
public:
    /** Returns whether the passed Attribute is a weight according to the file's metadata. */
    bool isWeight( Attribute* at );
    /** Returns the Attribute the given weight Attribute referes to according to the file's metadata.
     Returns a null pointer no such Attribute is found.*/
    Attribute* getVariableOfWeight( Attribute* weight );
    virtual void deleteVariable( uint columnToDelete );
	virtual bool isRegular() { return false; }
	virtual double getDataSpatialLocation( uint line, CartesianCoord whichCoord );
    virtual void   getDataSpatialLocation( uint line, double& x, double& y, double& z );
    virtual bool isTridimensional();
    virtual bool getCenter( double& x, double& y, double& z ) const;

    // File interface
public:
    bool canHaveMetaData();
    QString getFileType() const;
    void updateMetaDataFile();
    bool isDataFile(){ return true; }
	bool isDistribution(){ return false; }

    // ProjectComponent interface
public:
    QIcon getIcon();
    void save(QTextStream *txt_stream);
    virtual View3DViewData build3DViewObjects( View3DWidget * widget3D );

// ICalcPropertyCollection interface
public:
	virtual void getSpatialAndTopologicalCoordinates(int iRecord, double& x, double& y, double& z, int& i, int& j, int& k );
	virtual double getNeighborValue( int iRecord, int iVar, int dI, int dJ, int dK );


protected:
    int _x_field_index; //index start at 1. Zero means not set.
    int _y_field_index;
    int _z_field_index;
    QMap<uint, uint> _wgt_var_pairs; //pairs relating weights (firs uint) and variables (second uint
                                     //) by their GEO-EAS indexes (1=first)
};

#endif // POINTSET_H
