#include "gridcell.h"

#include "domain/cartesiangrid.h"

#include <cmath>

GridCell::GridCell() :
    _grid(nullptr), _i(0), _j(0), _k(0), _dataIndex(0)
{
    _centerX = 0.0;
    _centerY = 0.0;
    _centerZ = 0.0;
}

GridCell::GridCell(CartesianGrid *grid, int dataIndex, int i, int j, int k) :
    _grid(grid), _i(i), _j(j), _k(k), _dataIndex(dataIndex)
{
    _centerX = grid->getX0() + _i * grid->getDX();
    _centerY = grid->getY0() + _j * grid->getDY();
    _centerZ = grid->getZ0() + _k * grid->getDZ();
}

int GridCell::computeTopoDistance(GridCell &fromCell)
{
    return std::abs( _i - fromCell._i ) +
           std::abs( _j - fromCell._j ) +
           std::abs( _k - fromCell._k );
}
