#include "ijspatiallocation.h"
#include <cmath>
#include <iostream>

IJSpatialLocation::IJSpatialLocation() :
	_x(0.0), _y(0.0), _z(0.0)
{
}

IJSpatialLocation::IJSpatialLocation(double x, double y, double z) :
    _x(x), _y(y), _z(z)
{

}

double IJSpatialLocation::crossProduct2D(const IJSpatialLocation &otherLocation)
{
	return _x*otherLocation._y - _y*otherLocation._x;
}

double IJSpatialLocation::norm2D()
{
    return std::sqrt( _x*_x + _y*_y );
}

double IJSpatialLocation::norm()
{
    return std::sqrt( _x*_x + _y*_y + _z*_z );
}

IJSpatialLocation IJSpatialLocation::operator-(const IJSpatialLocation &b) const
{
    return IJSpatialLocation( _x - b._x, _y - b._y, _z - b._z );
}

void IJSpatialLocation::print()
{
    std::cout << "x=" << _x << " y=" << _y << " z=" << _z << std::endl;
}
