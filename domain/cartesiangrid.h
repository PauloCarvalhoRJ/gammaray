#ifndef CARTESIANGRID_H
#define CARTESIANGRID_H

#include "datafile.h"
#include "geostats/spatiallocation.h"
#include "imagejockey/ijabstractcartesiangrid.h"
#include <set>

class GSLibParGrid;
class GridCell;
class SVDFactor;

//third-party library eigen
namespace spectral{
   class array;
}

class CartesianGrid : public DataFile, public IJAbstractCartesianGrid
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
	uint getNX() const { return _nx; }
	uint getNY() const { return _ny; }
	uint getNZ() const { return _nz; }
	double getRot() const { return _rot; }
	uint getNReal() const { return _nreal; }
    //@}

    /**
     * Returns a value from the data column (0 = 1st column) given a grid topological coordinate (IJK).
     * @param i must be between 0 and NX-1.
     * @param j must be between 0 and NY-1.
     * @param k must be between 0 and NZ-1.
     */
    inline double dataIJK(uint column, uint i, uint j, uint k){
        uint dataRow = i + j*_nx + k*_ny*_nx;
        return data( dataRow, column );
    }

    /** Creates a vector of complex numbers with the values taken from data columns.
     *  Specify -1 to omit a column, which causes the repective part to be filled with zeros.
     *  getArray(-1,-1) returns an array filled with zeroes.  The dimension of the array is that
     *  of the Cartesian grid (getNX(), getNY(), getNZ()).
     *  @param indexColumRealPart Column index (starting with 0) with the values for the real part.
     *  @param indexColumRealPart Column index (starting with 0) with the values for the imaginary part.
     */
    std::vector< std::complex<double> > getArray( int indexColumRealPart, int indexColumImaginaryPart = -1 );

    /** Returns a data array skipping elements in the original data.
     *  Passing rates 1,1,1 results in a simple copy.
     *  This function is useful with large grids to produce a data subset for quick visualization,
     *  histogram computing, etc.
     *  The resulting grid dimensions are stored in the output parameters finalN*
     */
    std::vector< std::vector<double> > getResampledValues(int rateI, int rateJ, int rateK ,
                                                          int &finalNI, int &finalNJ, int &finalNK);

    /** Returns the value of the variable (given by its zero-based index) at the given spatial location.
     *  The function returns the value of grid cell that contains the given location.  The z coordinate is ignored
     * if the grid is 2D.
     * Make sure you load the desired realization with DataFile::setDataPage(), otherwise the value of the first
     * realization will be returned.
     * @param logOnError Enables error logging.  Enabling this might cause overflow error if called frequently
     *        with invalid coordinates, which is common when called from framework callbacks (e.g. grid rendering)
     */
    double valueAt(uint dataColumn, double x, double y, double z, bool logOnError = false);

    /**
     * Make a call to DataFile::setDataPage() such that only the given realization number is loaded into memory.
     * First realization is number 0 (zero).  To restore the default behavior (load entire data), call
     * DataFile::setDataPageToAll().
     */
    void setDataPageToRealization( uint nreal );

    /** Returns the grid's center. */
    SpatialLocation getCenter();

    /**
     * Returns, via output variables (i,j and k), the IJK coordinates corresponding to a XYZ spatial coordinate.
     * Returns false if the spatial coordinate lies outside the grid.
     */
    bool XYZtoIJK( double x, double y, double z,
                   uint& i,   uint& j,   uint& k );

    /** Sets the number of realizations.
     * This is declarative only.  No check is performed whether there are actually the number of
     * realizations informed.
     */
    void setNReal( uint n );

    /** Adds de contents of the given data array as new column to this Cartesian grid. */
    long append( const QString columnName, const spectral::array& array );

	/** Converts a data row index into topological coordinates (output parameters). */
	void indexToIJK(uint index, uint & i, uint & j, uint & k );

	/** Replaces the data in the column with the data in passed data array. */
	void setColumnData( uint dataColumn, spectral::array& array );

    /** Translates the grid to the given origin. */
    void setOrigin( double x0, double y0, double z0 );

    //DataFile interface
public:
    /** Cartesian grids never have declustering weights.  At least they are not supposed to be. */
	virtual bool isWeight( Attribute* /*at*/ ) { return false; }
    /** Cartesian grids never have declustering weights.  At least they are not supposed to be. */
    virtual Attribute* getVariableOfWeight( Attribute* /*at*/ ) { return nullptr; }
	virtual bool isRegular() { return true; }

// File interface
public:
	virtual bool canHaveMetaData();
	virtual QString getFileType();
	virtual void updateMetaDataFile();
	virtual bool isDataFile(){ return true; }

// ProjectComponent interface
public:
	virtual QIcon getIcon();
	virtual void save(QTextStream *txt_stream);
    virtual View3DViewData build3DViewObjects( View3DWidget * widget3D );

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

// ICalcPropertyCollection interface
public:
	virtual void getSpatialAndTopologicalCoordinates(int iRecord, double& x, double& y, double& z, int& i, int& j, int& k );
	virtual double getNeighborValue( int iRecord, int iVar, int dI, int dJ, int dK );

private:
    double _x0, _y0, _z0, _dx, _dy, _dz, _rot;
    uint _nx, _ny, _nz, _nreal;

    /**
     * Sets a value in the data column (0 = 1st column) given a grid topological coordinate (IJK).
     * @param i must be between 0 and NX-1.
     * @param j must be between 0 and NY-1.
     * @param k must be between 0 and NZ-1.
     */
    void setDataIJK( uint column, uint i, uint j, uint k, double value );
};

#endif // CARTESIANGRID_H
