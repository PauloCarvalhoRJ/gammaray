#ifndef GRIDCELL_H
#define GRIDCELL_H

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
    GridCell( CartesianGrid* grid, int dataIndex, int i, int j, int k );

    /** Computes the topological distance from the given cell.
     * The result is also stored in _topoDistance member variable.
     */
    int computeTopoDistance( GridCell &fromCell );

//--------------------member variables---------------------
    /** The grid object this cell refers to. */
    CartesianGrid* _grid;

    /** Topological coordinates. */
    int _i, _j, _k;

    /** Spatial coordinates of the cell center. */
    double _centerX, _centerY, _centerZ;

    /** Data index in multi-valued cells. */
    int _dataIndex;

    /** Topological distance computed with computeTopoDistance(); */
    int _topoDistance;
};

/**
 * This global non-member less-than operator enables the GridCell class as key-able
 * in STL or STL-like ordered containers.
 */
inline bool operator<(const GridCell &e1, const GridCell &e2){
    return e1._topoDistance < e2._topoDistance;
}

#endif // GRIDCELL_H
