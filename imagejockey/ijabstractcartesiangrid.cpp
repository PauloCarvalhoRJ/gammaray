#include "ijabstractcartesiangrid.h"
#include "imagejockeyutils.h"
#include <cmath>
#include "spectral/spectral.h"

IJAbstractCartesianGrid::IJAbstractCartesianGrid()
{
}

double IJAbstractCartesianGrid::getDiagonalLength()
{
    //TODO: add support for rotations
    if( ! ImageJockeyUtils::almostEqual2sComplement( getRotation(), 0.0, 1) ){
        return std::numeric_limits<double>::quiet_NaN();
    }
    double x0 = getOriginX() - getCellSizeI() / 2.0d;
    double y0 = getOriginY() - getCellSizeJ() / 2.0d;
    double z0 = getOriginZ() - getCellSizeK() / 2.0d;
    double xf = x0 + getCellSizeI() * getNI();
    double yf = y0 + getCellSizeJ() * getNJ();
    double zf = z0 + getCellSizeK() * getNK();
    double dx = xf - x0;
    double dy = yf - y0;
    double dz = zf - z0;
    return std::sqrt( dx*dx + dy*dy + dz*dz );
}

double IJAbstractCartesianGrid::getCenterX() const
{
    return getCenterLocation()._x;
}

double IJAbstractCartesianGrid::getCenterY() const
{
    return getCenterLocation()._y;
}

double IJAbstractCartesianGrid::getCenterZ() const
{
    return getCenterLocation()._z;
}

IJSpatialLocation IJAbstractCartesianGrid::getCenterLocation() const
{
    IJSpatialLocation result;
    //TODO: add support for rotations
    if( ! ImageJockeyUtils::almostEqual2sComplement( getRotation(), 0.0, 1) ){
        double errorValue = std::numeric_limits<double>::quiet_NaN();
        result._x = errorValue;
        result._y = errorValue;
        result._z = errorValue;
        return result;
    }
    double x0 = getOriginX() - getCellSizeI() / 2.0d;
    double y0 = getOriginY() - getCellSizeJ() / 2.0d;
    double z0 = getOriginZ() - getCellSizeK() / 2.0d;
    double xf = getOriginX() + getCellSizeI() * getNI();
    double yf = getOriginY() + getCellSizeJ() * getNJ();
    double zf = getOriginZ() + getCellSizeK() * getNK();
    result._x = (xf + x0) / 2.0;
    result._y = (yf + y0) / 2.0;
    result._z = (zf + z0) / 2.0;
    return result;
}
