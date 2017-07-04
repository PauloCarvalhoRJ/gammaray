#include "gridcell.h"

#include "domain/cartesiangrid.h"

#include <cmath>

GridCell::GridCell() :
    _grid(nullptr), _indexIJK(0,0,0), _dataIndex(0)
{
    _center._x = 0.0;
    _center._y = 0.0;
    _center._z = 0.0;
}

GridCell::GridCell(CartesianGrid *grid, int dataIndex, int i, int j, int k) :
    _grid(grid), _indexIJK(i,j,k), _dataIndex(dataIndex)
{
    _center._x = grid->getX0() + _indexIJK._i * grid->getDX();
    _center._y = grid->getY0() + _indexIJK._j * grid->getDY();
    _center._z = grid->getZ0() + _indexIJK._k * grid->getDZ();
}

int GridCell::computeTopoDistance(GridCell &fromCell)
{
    _topoDistance = std::abs( _indexIJK._i - fromCell._indexIJK._i ) +
                    std::abs( _indexIJK._j - fromCell._indexIJK._j ) +
                    std::abs( _indexIJK._k - fromCell._indexIJK._k );
    return _topoDistance;
}

double GridCell::readValueFromGrid() const
{
    return _grid->dataIJK( _dataIndex, _indexIJK._i, _indexIJK._j, _indexIJK._k );
}
