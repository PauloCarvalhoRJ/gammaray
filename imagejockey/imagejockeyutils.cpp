#include "imagejockeyutils.h"
#include <cstdlib>
#include <cmath>

/*static*/const long double ImageJockeyUtils::PI( 3.141592653589793238L );

/*static*/const long double ImageJockeyUtils::PI_OVER_180( ImageJockeyUtils::PI / 180.0L );

ImageJockeyUtils::ImageJockeyUtils()
{
}

double ImageJockeyUtils::dB( double value, double refLevel, double epsilon, double scaleFactor )
{
	double absValue = std::abs( value );
	double valueToUse = value;
	if( absValue < epsilon ){
		if( value < 0.0 )
			valueToUse = -epsilon;
		else
			valueToUse = epsilon;
	}
	return scaleFactor * std::log10( valueToUse / refLevel );
}

IJMatrix3X3<double> ImageJockeyUtils::getAnisoTransform(double aSemiMajor,
                                                      double aSemiMinor,
                                                      double aSemiVert,
                                                      double azimuth,
                                                      double dip,
                                                      double roll)
{

    //convert the angles to radians and to trigonometric convention
    double azimuthRad = (azimuth - 90.0) * PI_OVER_180;
    double dipRad = dip * PI_OVER_180;
    double rollRad = roll * PI_OVER_180;

    //----------rotate the world so the aniso axes are parallel to world axes--------------------------
    IJMatrix3X3<double> Tyaw( std::cos(azimuthRad), -std::sin(azimuthRad), 0.0,
                            std::sin(azimuthRad), std::cos(azimuthRad),  0.0,
                            0.0,               0.0,                1.0);
    IJMatrix3X3<double> Tpitch( std::cos(dipRad),  0.0, std::sin(dipRad),
                              0.0,            1.0,           0.0,
                              -std::sin(dipRad), 0.0, std::cos(dipRad));
    IJMatrix3X3<double> Troll( 1.0,            0.0,             0.0,
                             0.0, std::cos(rollRad), std::sin(rollRad),
                             0.0, -std::sin(rollRad), std::cos(rollRad));
    //----------stretches the world so the aniso ranges are now equal (spherical) --------------------
    IJMatrix3X3<double> S( 1.0,                   0.0,                  0.0,
                         0.0, aSemiMajor/aSemiMinor,                  0.0,
                         0.0,                   0.0, aSemiMajor/aSemiVert);
    //     <--------- order of transform application
    //  the final effect is that we transform the world so the anisotropy becomes isotropy
    return       S * Troll * Tpitch * Tyaw;
}

void ImageJockeyUtils::transform(IJMatrix3X3<double> &t, double &a1, double &a2, double &a3)
{
    double temp_a1 = t._a11 * a1 + t._a12 * a2 + t._a13 * a3;
    double temp_a2 = t._a21 * a1 + t._a22 * a2 + t._a23 * a3;
    double temp_a3 = t._a31 * a1 + t._a32 * a2 + t._a33 * a3;
    a1 = temp_a1;
    a2 = temp_a2;
    a3 = temp_a3;
}
