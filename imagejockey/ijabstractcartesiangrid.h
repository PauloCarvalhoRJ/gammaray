#ifndef IJABSTRACTCARTESIANGRID_H
#define IJABSTRACTCARTESIANGRID_H

#include "ijspatiallocation.h"

#include <QString>
#include <QIcon>

class IJAbstractVariable;

namespace spectral {
    struct array;
    struct complex_array;
}

/**
 * The IJAbstractCartesianGrid class represents a Cartesian grid in the Image Jockey sub-system.
 */
class IJAbstractCartesianGrid
{
public:
    IJAbstractCartesianGrid();
	virtual ~IJAbstractCartesianGrid(){}

    /** Returns the length of the diagonal of the grid's box. */
    double getDiagonalLength();

    /** Returns the rotation about the Z-axis in the grid's origin. */
    virtual double getRotation() = 0;

    //@{
    /** Getters for the spatial location of the center of the grid's box. */
    virtual double getCenterX() = 0;
    virtual double getCenterY() = 0;
    virtual double getCenterZ() = 0;
	virtual IJSpatialLocation getCenterLocation() = 0;
    //@}

    //@{
    /** Getters for the grid parameters. */
    virtual int getNI() = 0;
    virtual int getNJ() = 0;
    virtual int getNK() = 0;
    virtual double getCellSizeI() = 0;
    virtual double getCellSizeJ() = 0;
    virtual double getCellSizeK() = 0;
    virtual double getOriginX() = 0;
    virtual double getOriginY() = 0;
    virtual double getOriginZ() = 0;
    //@}

    /** Returns the value of the variable given its index and the topological coordinate. */
    virtual double getData( int variableIndex, int i, int j, int k ) = 0;

    /** Returns whether the given value is considered uninformed datum. */
    virtual bool isNoDataValue( double value ) = 0;

    /** Returns the value of the variable (given by its zero-based index) at the given spatial location.
     *  The function returns the value of grid cell that contains the given location.  The z coordinate is ignored
     * if the grid is 2D.
     * Make sure you load/select the desired realization prior to calling this in multi-realization grids.
     */
	virtual double getDataAt( int variableIndex, double x, double y, double z ) = 0;

    /**
     * Returns the maximum absolute value in the given variable.
     * First variable is 0.
     */
    virtual double absMax( int variableIndex ) = 0;

    /**
     * Returns the minimum absolute value in the given variable.
     * First variable is 0.
     */
    virtual double absMin( int variableIndex ) = 0;

	/**
	 * This is called prior to data retrieval.
	 * This may be an opportunity to prefetch data from files or databases.
	 */
	virtual void dataWillBeRequested() = 0;

    /** Returns the name of the grid. This is mainly used in presentation (e.g. text in widgets). */
    virtual QString getGridName() = 0;

    /** Returns the icon of the grid. This is mainly used in presentation (e.g. icons in widgets). */
    virtual QIcon getGridIcon() = 0;

    /** Returns the index of a variable given its name. Implementations should return an invalid index
     * (e.g. negative) if no variable matches the passed name.
     */
    virtual int getVariableIndexByName( QString variableName ) = 0;

    /** Returns a variable given its name.  Implementations should return null pointer if no variables
     * matches the passed name.
     */
    virtual IJAbstractVariable* getVariableByName( QString variableName ) = 0;

    /**
     * Fills the passed vector with pointers to the grid's variables.
     */
    virtual void getAllVariables(  std::vector<IJAbstractVariable*>& result ) = 0;

    /**
     * Returns the variable given its index.  Implementations should return null pointer if there is no
     * variable with the given index or the index is invalid (e.g. negative).
     */
    virtual IJAbstractVariable* getVariableByIndex( int variableIndex ) = 0;

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
    virtual void equalizeValues(QList<QPointF>& area,
                                double delta_dB,
                                int variableIndex,
                                double dB_reference,
                                const QList<QPointF>& secondArea = QList<QPointF>()) = 0;

    /** Save data to persistence (file, database, etc.). */
    virtual void saveData() = 0;

    /** Creates a spectral::array object from a variable of this Cartesian grid.
     * The client code is responsible for managing the memory occupied by the object.
     * Implementations must include spectral/spectral.h header.
     */
    virtual spectral::array* createSpectralArray( int variableIndex ) = 0;

    /** Creates a spectral::complex_array object from two variables of this Cartesian grid.
     * The client code is responsible for managing the memory occupied the object.
     * Implementations must include spectral/spectral.h header.
     * @note Beware of the form of the complex numbers, the first variable may be a magnitude
     * and the second variable may be the phase if the complex numbers are in polar form.
     */
    virtual spectral::complex_array* createSpectralComplexArray( int variableIndex1,
                                                                 int variableIndex2) = 0;

    /**
     * Clear data loaded to memory.
     */
    virtual void clearLoadedData() = 0;

    /** Adds de contents of the given data array as new variable to this Cartesian grid. */
    virtual long appendAsNewVariable( const QString variableName, const spectral::array& array ) = 0;
};

#endif // IJABSTRACTCARTESIANGRID_H
