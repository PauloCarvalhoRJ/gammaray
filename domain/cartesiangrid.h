#ifndef CARTESIANGRID_H
#define CARTESIANGRID_H

#include "gridfile.h"
#include "imagejockey/ijabstractcartesiangrid.h"
#include <set>

class GSLibParGrid;
class GridCell;
class SVDFactor;
class SpatialLocation;
class GeoGrid;

class CartesianGrid : public GridFile, public IJAbstractCartesianGrid
{
public:
    CartesianGrid( QString path );
    /** Sets cartesian grid metadata.  It also populates the file's attribute collection. */
    void setInfo(double x0, double y0, double z0,
                  double dx, double dy, double dz,
                  int nx, int ny, int nz,
                  double rot, int nreal, const QString no_data_value,
                  QMap<uint, QPair<uint, QString> > nvar_var_trn_triads,
                  const QList<QPair<uint, QString> > &categorical_attributes);

    /** Sets cartesian grid metadata from the accompaining .md file, if it exists.
     Nothing happens if the metadata file does not exist.  If it exists, it calls
     #setInfo() with the metadata read from the .md file.*/
    void setInfoFromMetadataFile();

    /** Sets the cartesian grid metadata by copying the values from another grid, specified by
     * the given pointer.
     * @param copyCategoricalAttributesList If false, unlink any categorical variables to category definitions
     *        turning them into common attributes, interpreted as continuous values.
     */
    void setInfoFromOtherCG( CartesianGrid* other_cg, bool copyCategoricalAttributesList = true );

	/** Sets the cartesian grid metadata with the grid parameters of the passed grid object.
	 */
	void setInfoFromSVDFactor( const SVDFactor* factor );

    /**
     * Sets the cartesian grid metadata from the values in a grid paraemeter object.
     */
    void setInfoFromGridParameter( GSLibParGrid* pg );

	/**
	 * Sets the cartesian grid metadata from the grid parameters of a GeoGrid (possibly the parent stratigraphic mesh).
	 */
	void setInfoFromGeoGrid( GeoGrid* gg );

    /** Changes the grid metadata to the new cell dimensions/count.
     */
    void setCellGeometry( int nx, int ny, int nz, double dx, double dy, double dz );

    //@{
    /** Getters. */
	double getX0() const { return _x0; }
	double getY0() const { return _y0; }
	double getZ0() const { return _z0; }
	double getDX() const { return _dx; }
	double getDY() const { return _dy; }
	double getDZ() const { return _dz; }
	uint getNX() const { return m_nI; }
	uint getNY() const { return m_nJ; }
	uint getNZ() const { return m_nK; }
	double getRot() const { return _rot; }
	uint getNReal() const { return m_nreal; }
    //@}

    /** Returns a data array skipping elements in the original data.
     *  Passing rates 1,1,1 results in a simple copy.
     *  This function is useful with large grids to produce a data subset for quick visualization,
     *  histogram computing, etc.
     *  The resulting grid dimensions are stored in the output parameters finalN*
     */
    std::vector< std::vector<double> > getResampledValues(int rateI, int rateJ, int rateK ,
                                                          int &finalNI, int &finalNJ, int &finalNK);


    /** Translates the grid to the given origin. */
    void setOrigin( double x0, double y0, double z0 );

	/** Returns whether this Cartesian grid is beind used as data store for a GeoGrid.
	 * In other words: returns whether this grid is child of a GeoGrid file in the project tree.
	 */
	bool isUVWOfAGeoGrid();

    /** Returns whether this Cartesian grid is beind used as data store for a Section.
     * In other words: returns whether this grid is child of a Section file in the project tree.
     */
    bool isDataStoreOfaGeologicSection();

//GridFile interface
	virtual bool XYZtoIJK( double x, double y, double z,
						   uint& i,   uint& j,   uint& k );
	virtual SpatialLocation getCenter();
	virtual void IJKtoXYZ( uint i,    uint j,    uint k,
                           double& x, double& y, double& z ) const;


//DataFile interface
public:
    /** Cartesian grids never have declustering weights.  At least they are not supposed to be. */
	virtual bool isWeight( Attribute* /*at*/ ) { return false; }
    /** Cartesian grids never have declustering weights.  At least they are not supposed to be. */
    virtual Attribute* getVariableOfWeight( Attribute* /*at*/ ) { return nullptr; }
	virtual bool isRegular() { return true; }
	virtual double getDataSpatialLocation( uint line, CartesianCoord whichCoord );
    virtual void   getDataSpatialLocation( uint line, double& x, double& y, double& z );
    virtual bool isTridimensional();

// File interface
public:
	virtual bool canHaveMetaData();
    virtual QString getFileType() const;
	virtual void updateMetaDataFile();
	virtual bool isDataFile(){ return true; }
	bool isDistribution(){ return false; }

// ProjectComponent interface
public:
	virtual QIcon getIcon();
	virtual void save(QTextStream *txt_stream);
    virtual View3DViewData build3DViewObjects( View3DWidget * widget3D );
	virtual QString getPresentationName();

//IJAbstractCartesianGrid interface
public:
	virtual double getRotation() const;
	virtual int getNI() const { return getNX(); }
	virtual int getNJ() const { return getNY(); }
	virtual int getNK() const { return getNZ(); }
	virtual double getCellSizeI() const { return getDX(); }
	virtual double getCellSizeJ() const { return getDY(); }
	virtual double getCellSizeK() const { return getDZ(); }
	virtual double getOriginX() const { return getX0(); }
	virtual double getOriginY() const { return getY0(); }
	virtual double getOriginZ() const { return getZ0(); }
    virtual double getData( int variableIndex, int i, int j, int k );
    virtual bool isNoDataValue( double value );
    virtual double getDataAt( int dataColumn, double x, double y, double z );
    virtual double absMax( int column );
    virtual double absMin( int column );
	virtual void dataWillBeRequested();
	virtual QString getGridName() const;
    virtual QIcon getGridIcon();
    virtual int getVariableIndexByName( QString variableName );
    virtual IJAbstractVariable* getVariableByName( QString variableName );
    virtual void getAllVariables(  std::vector<IJAbstractVariable*>& result );
    virtual IJAbstractVariable* getVariableByIndex( int variableIndex );
    virtual void equalizeValues(QList<QPointF>& area, double delta_dB, int dataColumn, double dB_reference,
                        const QList<QPointF>& secondArea = QList<QPointF>());
    virtual void saveData();
	virtual spectral::array* createSpectralArray( int nDataColumn );
    virtual spectral::complex_array* createSpectralComplexArray( int variableIndex1,
                                                                 int variableIndex2);
    virtual void clearLoadedData();
    virtual long appendAsNewVariable( const QString variableName, const spectral::array& array );
    virtual double getUninformedDataValue(){ return getNoDataValueAsDouble(); }
    virtual void getCellLocation(int i, int j, int k, double& x, double& y, double& z) const;
    virtual double getMax( int column );
    virtual double getMin( int column );

// ICalcPropertyCollection interface
public:
	virtual void getSpatialAndTopologicalCoordinates(int iRecord, double& x, double& y, double& z, int& i, int& j, int& k );
    virtual void computationWillStart();

private:
    double _x0, _y0, _z0, _dx, _dy, _dz, _rot;

};

#endif // CARTESIANGRID_H
