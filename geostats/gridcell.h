#ifndef GRIDCELL_H
#define GRIDCELL_H

#include "datacell.h"
#include "ijkindex.h"
#include "domain/gridfile.h"

/** Data structure containing information of a grid cell. */
class GridCell : public DataCell
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
    inline GridCell( GridFile* grid, int dataIndex, int i, int j, int k ) : DataCell( dataIndex, grid ),
		_grid(grid), _indexIJK(i,j,k)
    {
        grid->IJKtoXYZ( i, j, k, _center._x, _center._y, _center._z);

    }

    /** Computes the topological distance from the given cell.
     * The result is also stored in _topoDistance member variable.
     */
    inline int computeTopoDistance( const GridCell &fromCell ){
        _topoDistance = std::abs( _indexIJK._i - fromCell._indexIJK._i ) +
                        std::abs( _indexIJK._j - fromCell._indexIJK._j ) +
                        std::abs( _indexIJK._k - fromCell._indexIJK._k );
        return _topoDistance;
    }

    /** The grid object this cell refers to. */
    GridFile* _grid;

    /** Topological coordinates (i,j,k). */
    IJKIndex _indexIJK;

    /** Topological distance computed with computeTopoDistance(); */
    int _topoDistance;

    /** Returns the value from the grid associated with this cell.
     * It assumes all cell info are correct.
    */
    double readValueFromGrid() const;

    /** Returns the value from the grid associated with this cell given the index
     * of anu variable.
     * It assumes all cell info are correct (_dataIndex does not need to be set).
     */
    double readValueFromGrid( uint dataColumnIndex ) const;

// DataCell	interface
	virtual double readValueFromDataSet() const{
		return readValueFromGrid();
	}
    virtual double readValueFromDataSet( unsigned int dataColumnIndex ) const{
        return readValueFromGrid( dataColumnIndex );
    }
    virtual uint64_t getDataRowIndex() const {
        return _grid->IJKtoIndex( _indexIJK._i, _indexIJK._j, _indexIJK._k );
    }
};

typedef std::shared_ptr<GridCell> GridCellPtr;

/** The GridCellPtr comparator class. */
struct GridCellPtrComparator
{
	bool operator()(const GridCellPtr &d1, const GridCellPtr &d2) const {
		return d1->_topoDistance < d2->_topoDistance;
	}
};

/** An ordered container by the GridCell's topological distance. */
typedef std::multiset<GridCellPtr, GridCellPtrComparator> GridCellPtrMultiset;


#endif // GRIDCELL_H
