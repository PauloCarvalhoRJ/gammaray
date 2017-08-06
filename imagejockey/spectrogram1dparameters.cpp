#include "spectrogram1dparameters.h"

#include <cmath>

#include "util.h"
#include "geostats/matrix3x3.h"
#include "geostats/geostatsutils.h"

Spectrogram1DParameters::Spectrogram1DParameters() :
    ExperimentalVariogramParameters()
{
}
double Spectrogram1DParameters::radius() const
{
    return _radius;
}

void Spectrogram1DParameters::setRadius(double radius)
{
    _radius = radius;
    updateGeometry();
    emit updated();
}
double Spectrogram1DParameters::endRadius() const
{
    return _endRadius;
}

int Spectrogram1DParameters::getNPointsPerBandIn2DGeometry() const
{
    return _n2DBandPoints;
}

void Spectrogram1DParameters::setEndRadius(double endRadius)
{
    _endRadius = endRadius;
    updateGeometry();
    emit updated();
}

void Spectrogram1DParameters::updateGeometry()
{
    //Update the two-band map view (2D) geometry.
    const int n = _n2DBandPoints;
    double x[n];
    double y[n];

    //get the band geometry from the parameters
    double x0 = refCenter()._x;
    double y0 = refCenter()._y;
    double gridExtent = endRadius();
    double radiusp = radius();
    double bandw = bandWidth();
    double ySect = bandw * std::tan( Util::PI/2.0d - azimuthTolerance() * Util::PI_OVER_180 ); //90deg - azimuth
    double azimuthp = -azimuth() - 90.0d;

    //==========================SET BAND 1=======================================

    //get the 1 semi-band geometry
    x[0] = - bandw;  y[0] = gridExtent + radiusp;
    x[1] = x[0];     y[1] = ySect + radiusp;
    x[2] = 0.0;      y[2] = radiusp;
    x[3] = bandw;    y[3] = y[1];
    x[4] = x[3];     y[4] = y[0];
    x[5] = x[0];     y[5] = y[0];

    //rotates the geometry towards the desired azimuth and
    //translates the geometry to the grid's center
    Matrix3X3<double> xform = GeostatsUtils::getAnisoTransform( 1.0, 1.0, 1.0, azimuthp, 0.0, 0.0 );
    double not_used_in_2D;
    for(int i = 0; i < n; ++i){
        GeostatsUtils::transform( xform, x[i], y[i], not_used_in_2D );
        _2DBand1x[i] = x[i] + x0;
        _2DBand1y[i] = y[i] + y0;
    }

    //==========================SET BAND 2=======================================

    //the other semi-band is symmetrical
    x[0] = - bandw;  y[0] = -gridExtent - radiusp;
    x[1] = x[0];     y[1] = -ySect - radiusp;
    x[2] = 0.0;      y[2] = -radiusp;
    x[3] = + bandw;  y[3] = y[1];
    x[4] = x[3];     y[4] = y[0];
    x[5] = x[0];     y[5] = y[0];

    //rotates the geometry towards the desired azimuth and
    //translates the geometry to the grid's center
    for(int i = 0; i < n; ++i){
        GeostatsUtils::transform( xform, x[i], y[i], not_used_in_2D );
        _2DBand2x[i] = x[i] + x0;
        _2DBand2y[i] = y[i] + y0;
    }
}


