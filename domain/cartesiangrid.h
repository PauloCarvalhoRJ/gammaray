#ifndef CARTESIANGRID_H
#define CARTESIANGRID_H

#include "datafile.h"

class GSLibParGrid;

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

private:
    double _x0, _y0, _z0, _dx, _dy, _dz, _rot;
    uint _nx, _ny, _nz, _nreal;
};

#endif // CARTESIANGRID_H
