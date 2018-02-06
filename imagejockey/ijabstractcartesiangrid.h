#ifndef IJABSTRACTCARTESIANGRID_H
#define IJABSTRACTCARTESIANGRID_H

/**
 * The IJAbstractCartesianGrid class represents a Cartesian grid in the Image Jockey sub-system.
 */
class IJAbstractCartesianGrid
{
public:
    IJAbstractCartesianGrid();
	virtual ~IJAbstractCartesianGrid(){}

    /** Returns the length of the diagonal of the grid's box. */
    virtual double getDiagonalLength() = 0;

    /** Returns the rotation about the Z-axis in the grid's origin. */
    virtual double getRotation() = 0;

    //@{
    /** Getters for the spatial location of the center of the grid's box. */
    virtual double getCenterX() = 0;
    virtual double getCenterY() = 0;
    virtual double getCenterZ() = 0;
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
};

#endif // IJABSTRACTCARTESIANGRID_H
