#include "geostatsutils.h"

#include <cmath>
#include <limits>

GeostatsUtils::GeostatsUtils()
{
}

Matrix3X3<double> GeostatsUtils::getAnisoTransform(double aSemiMajor,
                                                   double aSemiMinor,
                                                   double aSemiVert,
                                                   double azimuth,
                                                   double dip,
                                                   double roll)
{
    //----------rotate the world so the aniso axes are parallel to world axes--------------------------
    Matrix3X3<double> Tyaw( std::cos(azimuth), -std::sin(azimuth), 0.0,
                            std::sin(azimuth), std::cos(azimuth),  0.0,
                            0.0,               0.0,                1.0);
    Matrix3X3<double> Tpitch( std::cos(dip),  0.0, std::sin(dip),
                              0.0,            1.0,           0.0,
                              -std::sin(dip), 0.0, std::cos(dip));
    Matrix3X3<double> Troll( 1.0,            0.0,             0.0,
                             0.0, std::cos(roll), -std::sin(roll),
                             0.0, std::sin(roll), std::cos(roll));
    //----------stretches the world so the aniso ranges are now equal (spherical) --------------------
    Matrix3X3<double> S( 1.0,                   0.0,                  0.0,
                         0.0, aSemiMajor/aSemiMinor,                  0.0,
                         0.0,                   0.0, aSemiMajor/aSemiVert);
    //     <--------- order of transform application
    //  the final effect is that we transform the world so the anisotropy becomes an isotropy
    return       S * Troll * Tpitch * Tyaw;
}

void GeostatsUtils::transform(Matrix3X3<double> &t, double &a1, double &a2, double &a3)
{
    double temp_a1 = t._a11 * a1 + t._a12 * a2 + t._a13 * a3;
    double temp_a2 = t._a21 * a1 + t._a22 * a2 + t._a23 * a3;
    double temp_a3 = t._a31 * a1 + t._a32 * a2 + t._a33 * a3;
    a1 = temp_a1;
    a2 = temp_a2;
    a3 = temp_a3;
}

double GeostatsUtils::getH(double x0, double y0, double z0,
                           double x1, double y1, double z1,
                           Matrix3X3<double> &anisoTransform)
{
    double dx = x1 - x0;
    double dy = y1 - y0;
    double dz = z1 - z0;
    GeostatsUtils::transform( anisoTransform, dx, dy, dz );
    return std::sqrt( dx*dx + dy*dy + dz*dz );
}

double GeostatsUtils::getGamma(VariogramModel model, double h, double range, double contribution)
{
    switch( model ){
    case VariogramModel::SPHERICAL:
        if( h > range )
            return contribution;
        double h_over_a = h/range;
        return contribution * ( 1.5*h_over_a - 0.5*(h_over_a*h_over_a*h_over_a) );
        break;
    }
    return std::numeric_limits<double>::quiet_NaN();
}
