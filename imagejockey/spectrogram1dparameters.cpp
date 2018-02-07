#include "spectrogram1dparameters.h"

#include <cmath>
#include <QPointF>

#include "imagejockeyutils.h"
#include "ijmatrix3x3.h"

Spectrogram1DParameters::Spectrogram1DParameters() :
    IJExperimentalVariogramParameters()
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

double Spectrogram1DParameters::distanceToAxis(double x, double y)
{
    SpatialLocation end;
    SpatialLocation point;
    point._x = x;
    point._y = y;

    //translate the key locations to origin
    end._x = _aPointOnAxis._x - _refCenter._x;
    end._y = _aPointOnAxis._y - _refCenter._y;
    point._x -= _refCenter._x;
    point._y -= _refCenter._y;

    //origin, point and end locations form a triangle.
    double area = point.crossProduct2D( end );

    //you can tell which side of the axis the point is by the sign of area.
    //if area is zero, then the point is exactly on the axis.
    return std::abs( area / end.norm2D() );
}

QList<QPointF> Spectrogram1DParameters::getAreaOfInfluence(double centerFrequency, double frequencySpan)
{
    QList<QPointF> result;

    //convert the azimuth into a trigonometric angle in radians
    double azimuthp = -_azimuth - 90.0d ;
    double angleRadians = ( azimuthp ) * ImageJockeyUtils::PI_OVER_180;

    // Center frequency and azimuth can by regarded as the radius and angle of polar coordinate in a 2D spectrogram.
    // then we convert them to x,y to get a 2D Cartesian location.
    double centerX = _refCenter._x + centerFrequency * std::cos( angleRadians );
    double centerY = _refCenter._y + centerFrequency * std::sin( angleRadians );

    //define a non-rotated rectangle (zero azimuth) centered at origin with width (east-west size) corresponding
    //to the bandwidth and with length (north-south size) corresponding to the frequency span.
    double halfFreqSpan = frequencySpan / 2.0d;
    result.push_back( QPointF( - _bandWidth,   halfFreqSpan ) );
    result.push_back( QPointF(   _bandWidth,   halfFreqSpan ) );
    result.push_back( QPointF(   _bandWidth, - halfFreqSpan ) );
    result.push_back( QPointF( - _bandWidth, - halfFreqSpan ) );
    result.push_back( QPointF( - _bandWidth,   halfFreqSpan ) );

    //rotates the rectangle towards the desired azimuth and
    //translates it the position corresponding to the 1D center frequency
    IJMatrix3X3<double> xform = ImageJockeyUtils::getAnisoTransform( 1.0, 1.0, 1.0, azimuthp, 0.0, 0.0 );
    double not_used_in_2D;
    for(int i = 0; i < 5; ++i){
        double x = result[i].x();
        double y = result[i].y();
        ImageJockeyUtils::transform( xform, x, y, not_used_in_2D );
        result[i].setX( x + centerX );
        result[i].setY( y + centerY );
    }

    return result;
}

QList<QPointF> Spectrogram1DParameters::getHalfBandGeometry()
{
    QList<QPointF> result;
    const std::size_t n = getNPointsPerBandIn2DGeometry();
    result.reserve( n );
    for(std::size_t i = 0; i < n; ++i){
        result.push_back( QPointF( _2DBand1x[i], _2DBand1y[i] ) );
    }
    return result;
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
    double ySect = bandw * std::tan( ImageJockeyUtils::PI/2.0d -
                                     azimuthTolerance() * ImageJockeyUtils::PI_OVER_180 ); //90deg - azimuth
    double azimuthp = -azimuth() - 90.0d;

    //==========================SET BAND 1=======================================

    //init the 1 semi-band geometry sitting on origin and aligned with y axis (north-south)
    //to make code less difficult to follow
    x[0] = -bandw;   y[0] = gridExtent + radiusp;
    x[1] = x[0];     y[1] = ySect + radiusp;
    x[2] = 0.0;      y[2] = radiusp;
    x[3] = bandw;    y[3] = y[1];
    x[4] = x[3];     y[4] = y[0];
    x[5] = x[0];     y[5] = y[0];

    //rotates the geometry towards the desired azimuth and
    //translates the geometry to the grid's center
    IJMatrix3X3<double> xform = ImageJockeyUtils::getAnisoTransform( 1.0, 1.0, 1.0, azimuthp, 0.0, 0.0 );
    double not_used_in_2D;
    for(int i = 0; i < n; ++i){
        ImageJockeyUtils::transform( xform, x[i], y[i], not_used_in_2D );
        _2DBand1x[i] = x[i] + x0;
        _2DBand1y[i] = y[i] + y0;
    }

    //============================SET A PONINT-ON-AXIS USEFUL FOR COMPUTATIONAL GEOMETRY================

    //init the point-on-axis some distance away from center along the y-axis, because the
    //axis of the 1D spectrogram calculcation band is initially aligned with y-axis
    _aPointOnAxis._x = 0.0;
    _aPointOnAxis._y = gridExtent;

    //also rotates and translates the point-on-axis so it sits where it should be
    ImageJockeyUtils::transform( xform, _aPointOnAxis._x, _aPointOnAxis._y, not_used_in_2D);
    _aPointOnAxis._x += x0;
    _aPointOnAxis._y += y0;

    //==========================SET BAND 2=======================================

    //the other semi-band is symmetrical
    x[0] = -bandw;   y[0] = -gridExtent - radiusp;
    x[1] = x[0];     y[1] = -ySect - radiusp;
    x[2] = 0.0;      y[2] = -radiusp;
    x[3] = bandw;    y[3] = y[1];
    x[4] = x[3];     y[4] = y[0];
    x[5] = x[0];     y[5] = y[0];

    //rotates the geometry towards the desired azimuth and
    //translates the geometry to the grid's center
    for(int i = 0; i < n; ++i){
        ImageJockeyUtils::transform( xform, x[i], y[i], not_used_in_2D );
        _2DBand2x[i] = x[i] + x0;
        _2DBand2y[i] = y[i] + y0;
    }
}


