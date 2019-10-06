#include "gridcell.h"

#include "domain/cartesiangrid.h"

#include <cmath>

GridCell::GridCell() : DataCell( 0 ),
	_grid(nullptr), _indexIJK(0,0,0)
{
    _center._x = 0.0;
    _center._y = 0.0;
    _center._z = 0.0;
}

double GridCell::readValueFromGrid() const
{
    return _grid->dataIJK( _dataIndex, _indexIJK._i, _indexIJK._j, _indexIJK._k );
}

double GridCell::readValueFromGrid(uint dataColumnIndex) const
{
    return _grid->dataIJK( dataColumnIndex, _indexIJK._i, _indexIJK._j, _indexIJK._k );
}
