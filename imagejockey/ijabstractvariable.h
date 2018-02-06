#ifndef IJABSTRACTVARIABLE_H
#define IJABSTRACTVARIABLE_H

class IJAbstractCartesianGrid;

/**
 * The IJAbstractVariable class represents a series of values in an IJAbstractCartesianGrid
 */
class IJAbstractVariable
{
public:
    IJAbstractVariable();
	virtual ~IJAbstractVariable(){}

    /** Returns the pointer to the parent Cartesian grid object. */
    virtual IJAbstractCartesianGrid* getParentGrid() = 0;

    /** Returns the variable's index in the parent grid.
     * The first variable should have index zero.
     */
    virtual int getIndexInParentGrid() = 0;
};

#endif // IJABSTRACTVARIABLE_H
