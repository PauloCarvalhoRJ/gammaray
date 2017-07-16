#ifndef GRIDCELL_H
#define GRIDCELL_H

#include "geostats/spatiallocation.h"
#include "geostats/ijkindex.h"
#include "domain/cartesiangrid.h"

class CartesianGrid;

/** Data structure containing information of a grid cell. */
class GridCell
{
public:

    /** Default constructor required by some STL containers.  Avoid using this in client code.
     * Creates a cell centered at 0,0,0 and with 0,0,0 as topological coordinates.
     * The refering grid is a null pointer and the data index is 0.
     */
    GridCell();

    /** Preferable constructor.
     * @param grid The grid this cell refers to.
     * @param dataIndex Data column index in grid used to get values from (starts with 0) as the grids are multivalued.
     * @param i Topological coordinate (inline/column)
     * @param j Topological coordinate (crossline/row)
     * @param k Topological coordinate (horizontal slice)
     */
    inline GridCell( CartesianGrid* grid, int dataIndex, int i, int j, int k ) :
        _grid(grid), _indexIJK(i,j,k), _dataIndex(dataIndex)
    {
        _center._x = grid->getX0() + _indexIJK._i * grid->getDX();
        _center._y = grid->getY0() + _indexIJK._j * grid->getDY();
        _center._z = grid->getZ0() + _indexIJK._k * grid->getDZ();
    }

    /** Computes the topological distance from the given cell.
     * The result is also stored in _topoDistance member variable.
     */
    inline int computeTopoDistance( GridCell &fromCell ){
        _topoDistance = std::abs( _indexIJK._i - fromCell._indexIJK._i ) +
                        std::abs( _indexIJK._j - fromCell._indexIJK._j ) +
                        std::abs( _indexIJK._k - fromCell._indexIJK._k );
        return _topoDistance;
    }

//--------------------member variables---------------------
    /** The grid object this cell refers to. */
    CartesianGrid* _grid;

    /** Topological coordinates (i,j,k). */
    IJKIndex _indexIJK;

    /** Spatial coordinates of the cell center. */
    SpatialLocation _center;

    /** Data index in multi-valued cells. */
    int _dataIndex;

    /** Topological distance computed with computeTopoDistance(); */
    int _topoDistance;

    /** Returns the value from the grid associated with this cell.
     * It assumes all cell info are correct.
    */
    double readValueFromGrid() const;
};

/**
 * This global non-member less-than operator enables the GridCell class as key-able
 * in STL or STL-like ordered containers.
 */
inline bool operator<(const GridCell &e1, const GridCell &e2){
    return e1._topoDistance < e2._topoDistance;
}

#endif // GRIDCELL_H
