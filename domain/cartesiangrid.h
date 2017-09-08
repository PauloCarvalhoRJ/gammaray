#ifndef CARTESIANGRID_H
#define CARTESIANGRID_H

#include "datafile.h"
#include "geostats/spatiallocation.h"
#include <set>

class GSLibParGrid;
class GridCell;

class CartesianGrid : public DataFile
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

    /**
     * Sets the cartesian grid metadata from the values in a grid paraemeter object.
     */
    void setInfoFromGridParameter( GSLibParGrid* pg );

    /** Changes the grid metadata to the new cell dimensions/count.
     */
    void setCellGeometry( int nx, int ny, int nz, double dx, double dy, double dz );

    //@{
    /** Getters. */
    double getX0(){ return _x0; }
    double getY0(){ return _y0; }
    double getZ0(){ return _z0; }
    double getDX(){ return _dx; }
    double getDY(){ return _dy; }
    double getDZ(){ return _dz; }
    uint getNX(){ return _nx; }
    uint getNY(){ return _ny; }
    uint getNZ(){ return _nz; }
    double getRot(){ return _rot; }
    uint getNReal(){ return _nreal; }
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

    /** Returns the length of the grid's box diagonal. */
    double getDiagonalLength();

    /** Returns the grid's center. */
    SpatialLocation getCenter();

    /** Amplifies (dB > 0) or attenuates (dB < 0) the values in the given data column (zero == first data column)
     * Amplification means that positive values increase and negative values decrease.
     * Attenuation means that values get closer to zero, so positive values decrease and negative
     * values increase.
     * @param area A set of points delimiting the area of the grid whithin the equalization will take place.
     * @param delta_dB The mplification or attenuation factor.
     * @param dataColumn The zero-based index of the data column containing the values to be equalized.
     * @param dB_reference The value corresponding to 0dB.
     * @param secondArea Another area used as spatial criterion.  If empty, this is not used.  If this area does
     *        not intersect the first area (area parameter) no cell will be selected.
     */
    void equalizeValues(QList<QPointF>& area, double delta_dB, uint dataColumn, double dB_reference,
                        const QList<QPointF>& secondArea = QList<QPointF>());

    /**
     * Returns, via output variables (i,j and k), the IJK coordinates corresponding to a XYZ spatial coordinate.
     * Returns false if the spatial coordinate lies outside the grid.
     */
    bool XYZtoIJK( double x, double y, double z,
                   uint& i,   uint& j,   uint& k );

    //DataFile interface
public:
    /** Cartesian grids never have declustering weights.  At least they are not supposed to be. */
    bool isWeight( Attribute* /*at*/ ) { return false; }
    /** Cartesian grids never have declustering weights.  At least they are not supposed to be. */
    virtual Attribute* getVariableOfWeight( Attribute* /*at*/ ) { return nullptr; }

// File interface
public:
    bool canHaveMetaData();
    QString getFileType();
    void updateMetaDataFile();
    bool isDataFile(){ return true; }

// ProjectComponent interface
public:
    QIcon getIcon();
    void save(QTextStream *txt_stream);
    virtual View3DViewData build3DViewObjects( View3DWidget * widget3D );

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
