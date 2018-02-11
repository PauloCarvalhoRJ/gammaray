#include "ijabstractcartesiangrid.h"
#include "imagejockeyutils.h"
#include <cmath>

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
