#ifndef GRIDCELL_H
#define GRIDCELL_H

/** Data structure containing information of a grid cell. */
class GridCell
{
public:
    GridCell();
    GridCell( int dataIndex, int i, int j, int k );

    /** Topological coordinates. */
    int _i, _j, _k;

    /** Spatial coordinates. */
    double _x, _y, _z;

    /** Data index in multi-valued cells. */
    int _dataIndex;
};

#endif // GRIDCELL_H
