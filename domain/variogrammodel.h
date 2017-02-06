#ifndef VARIOGRAMMODEL_H
#define VARIOGRAMMODEL_H

#include "file.h"
#include <QList>

/**
 * @brief The VariogramModel class represents a vmodel (GSLib progam) parameter file,
 *  which contains a variogram model, that was saved by the user in the project directory.
 */
class VariogramModel : public File
{
public:
    VariogramModel( const QString path );

    /**
     * Returns the total variance expressed by the variogram model.
     * It is the sum of nugget effect and the variance contributions
     * of each nested variogram structure.
     */
    double getSill();

    /**
     * Returns the nugget effect value, or the variance at h=0.
     * The nugget effect expresses the sample value uncertainty.
     */
    double getNugget();

    /**
     * Returns the number of variogram structures, not counting the nugget effect.
     */
    uint getNst();

    /** Returns the id of the variogram structure type (spherical, exponential, gaussian, etc.).
      * @param structure Number of the structure, first is zero, not counting the nugget effect.
      */
    uint getIt( int structure );

    /** Returns the variance contribution of a variance structure.
      * @param structure Number of the structure, first is zero, not counting the nugget effect.
      */
    double getCC( int structure );

    /** Returns the maximum horizontal variogram range.
      * This is equivalent to the horizontal semi-major axis of the anisotropy ellipsoid.
      * @param structure Number of the structure, first is zero, not counting the nugget effect.
      */
    double get_a_hMax( int structure );

    /** Returns the minimum horizontal variogram range.
    * This is equivalent to the horizontal semi-minor axis of the anisotropy ellipsoid.
    * @param structure Number of the structure, first is zero, not counting the nugget effect.
    */
    double get_a_hMin( int structure );

    /** Returns the vertical variogram range.
    * This is equivalent to the vertical semi-axis of the anisotropy ellipsoid.
    * @param structure Number of the structure, first is zero, not counting the nugget effect.
    */
    double get_a_vert( int structure );

    /**
     * Returns the azimuth of the semi-major axis of the variogram ellipsoid.
     * @param structure Number of the structure, first is zero, not counting the nugget effect.
     */
    double getAzimuth( int structure );

    /**
     * Returns the dip angle of the semi-major axis of the variogram ellipsoid.
     * @param structure Number of the structure, first is zero, not counting the nugget effect.
     */
    double getDip( int structure );

    /**
     * Returns the roll angle about the semi-major axis of the variogram ellipsoid.
     * @param structure Number of the structure, first is zero, not counting the nugget effect.
     */
    double getRoll( int structure );

// File interface
public:
    QString getFileType(){ return "VMODEL"; }
    virtual bool canHaveMetaData(){ return false; }
    virtual void updateMetaDataFile(){;}

// ProjectComponent interface
public:
    QIcon getIcon();
    void save(QTextStream *txt_stream);

private:
    void readParameters();

    /** These variables are set by readParameters(). */
    //@{
    double m_Sill;
    double m_Nugget;
    uint m_nst; //number of structures, not counting the nugget effect.
    QList<uint> m_it; //structure type (spherical, exponential, gaussian, etc.)
    QList<double> m_cc; //structure variance contribution
    QList<double> m_a_hMax;
    QList<double> m_a_hMin;
    QList<double> m_a_vert;
    QList<double> m_Azimuth;
    QList<double> m_Dip;
    QList<double> m_Roll;
    //@}

};

#endif // VARIOGRAMMODEL_H
