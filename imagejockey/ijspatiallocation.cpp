#include "ijspatiallocation.h"
#include <cmath>


IJSpatialLocation::IJSpatialLocation() :
	_x(0.0), _y(0.0), _z(0.0)
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
