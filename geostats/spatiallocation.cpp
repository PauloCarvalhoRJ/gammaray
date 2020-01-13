#include "spatiallocation.h"
#include <cmath>
#include <iostream>

SpatialLocation::SpatialLocation() :
    _x(0.0), _y(0.0), _z(0.0)
{
}

SpatialLocation::SpatialLocation(double x, double y, double z) :
     _x(x), _y(y), _z(z)
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

void SpatialLocation::print()
{
    std::cout << "Location: x=" << _x << ", y=" << _y << ", z=" << _z << std::endl;
}

double SpatialLocation::distanceTo(double x, double y, double z)
{
    double dx = x - _x;
    double dy = y - _y;
    double dz = z - _z;
    return std::sqrt( dx*dx + dy*dy + dz*dz );
}

SpatialLocation SpatialLocation::operator+(double a) const
{
	SpatialLocation result;
	result._x = _x + a;
	result._y = _y + a;
	result._z = _z + a;
	return result;
}
