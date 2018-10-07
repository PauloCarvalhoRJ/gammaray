#ifndef VARIOGRAMMODEL_H
#define VARIOGRAMMODEL_H

#include "file.h"
#include <QList>
#include <QDateTime>

/*! Variogram structure types. */
enum class VariogramStructureType : int {
    SPHERIC = 1,
    EXPONENTIAL,
    GAUSSIAN,
    POWER_LAW,
    COSINE_HOLE_EFFECT
};

/**
 * @brief The VariogramModel class represents a vmodel (GSLib progam) parameter file,
 *  which contains a variogram model, that was saved by the user in the project directory.
 */
class VariogramModel : public File
{
public:
    VariogramModel( const QString path );

	/** Constructor for file-less variogram models. */
	VariogramModel( );

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
    VariogramStructureType getIt( int structure );

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

    /** Returns the highest semi-major axis range amongst the nested structures. */
    double get_max_hMax();

    /** Returns the highest semi-minor axis range amongst the nested structures. */
    double get_max_hMin();

    /** Returns the highest vertical axis range amongst the nested structures. */
    double get_max_vert();

	/** Returns a VariogramModel object from one of its structures.
	 * @note IMPORTANT: differently from the other methods that take a structure index.
	 *                  Zero index here corresponds to the nugget effect.
	 */
	VariogramModel makeVModelFromSingleStructure( int structure );

	/**
	 * Returns the number of variogram structures, including the nugget effect.
	 */
	uint getNstWithNugget();

    /** Returns a descriptive text for one of the variogram structures. */
    QString getStructureDescription( int structure );

    /** Returns whether this variogram model is a pure nugget model. */
    bool isPureNugget();

	bool forceReread() const;

    /** Sets whether the getters call readParameters() automatically.
     * Setting true, getters ensture an updated read with respect to the file, but results in a slow execution.
     * Setting false, getters only return the values in the member varaibles, which can be intersting in
     * performance critical code, but at risk of outdated information if the file is expected to change.
     * By default, _forceReread is true.
     */
    void setForceReread(bool forceReread);

    /** Reads the variogram model parameters from file. */
    void readParameters();

	/** Returns a VariogramModel object from one all its structures but without the nugget effect.
	 */
	VariogramModel makeVModelWithoutNugget( );

// File interface
public:
    QString getFileType(){ return "VMODEL"; }
    virtual bool canHaveMetaData(){ return false; }
    virtual void updateMetaDataFile(){;}
    bool isDataFile(){ return false; }
	bool isDistribution(){ return false; }

// ProjectComponent interface
public:
    QIcon getIcon();
    void save(QTextStream *txt_stream);

private:

    /** These variables are set by readParameters(). */
    //@{
    double m_Sill;
    double m_Nugget;
    uint m_nst; //number of structures, not counting the nugget effect.
    QList<VariogramStructureType> m_it; //structure type (spherical, exponential, gaussian, etc.)
    QList<double> m_cc; //structure variance contribution
    QList<double> m_a_hMax;
    QList<double> m_a_hMin;
    QList<double> m_a_vert;
    QList<double> m_Azimuth;
    QList<double> m_Dip;
    QList<double> m_Roll;
    //@}

    /**
     * Stores the file timestamp in the last call to readParameters().
     * This time is used to detect whether there as a change in the file, to prevent
     * unnecessary reads.
     */
    QDateTime _lastModifiedDateTimeLastRead;

    /** If true, each get*() method calls readParameters(), which assures updating, but is slow.
     * If false, the getters only returns the values stores in the member variables, but is subject to return
     * outdated values.  By default this variable is true.
     */
    bool _forceReread;
};

#endif // VARIOGRAMMODEL_H
