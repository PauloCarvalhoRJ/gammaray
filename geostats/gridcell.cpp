#include "gridcell.h"

#include "domain/cartesiangrid.h"

#include <cmath>

GridCell::GridCell() :
    _grid(nullptr), _i(0), _j(0), _k(0), _dataIndex(0)
{
    _center._x = 0.0;
    _center._y = 0.0;
    _center._z = 0.0;
}

GridCell::GridCell(CartesianGrid *grid, int dataIndex, int i, int j, int k) :
    _grid(grid), _i(i), _j(j), _k(k), _dataIndex(dataIndex)
{
    _center._x = grid->getX0() + _i * grid->getDX();
    _center._y = grid->getY0() + _j * grid->getDY();
    _center._z = grid->getZ0() + _k * grid->getDZ();
}

int GridCell::computeTopoDistance(GridCell &fromCell)
{
    _topoDistance = std::abs( _i - fromCell._i ) +
                    std::abs( _j - fromCell._j ) +
                    std::abs( _k - fromCell._k );
    return _topoDistance;
}

double GridCell::readValueFromGrid() const
{
    return _grid->dataIJK( _dataIndex, _i, _j, _k );
}
