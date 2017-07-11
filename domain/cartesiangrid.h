#ifndef CARTESIANGRID_H
#define CARTESIANGRID_H

#include "datafile.h"
#include <complex>
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
     * @brief setGeometryFromOtherCG
     * @param other_cg
     */
    void setGeometryFromOtherCG( CartesianGrid* other_cg );

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
    double dataIJK(uint column, uint i, uint j, uint k);

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
};

#endif // CARTESIANGRID_H
