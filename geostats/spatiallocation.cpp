#include "spatiallocation.h"
#include <cmath>

SpatialLocation::SpatialLocation() :
    _x(0.0), _y(0.0), _z(0.0)
{
}

double SpatialLocation::crossProduct2D(const SpatialLocation &otherLocation)
{
    return _x*otherLocation._y - _y*otherLocation._x;
}

double SpatialLocation::norm2D()
{
    return std::sqrt( _x*_x + _y*_y );
}
