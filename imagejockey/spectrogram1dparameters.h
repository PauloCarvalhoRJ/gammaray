#ifndef SPECTROGRAM1DPARAMETERS_H
#define SPECTROGRAM1DPARAMETERS_H

#include "geostats/experimentalvariogramparameters.h"

/** This class contains parameters to calculate a 1D spectrogram from a band in a 2D spectrogram. */
class Spectrogram1DParameters : public ExperimentalVariogramParameters
{

    Q_OBJECT

public:
    Spectrogram1DParameters();

    /** Returns the radius from the spectrogram center where the band starts outwardly. */
    double radius() const;

    /** Returns the radius from the spectrogram center where the band ends. */
    double endRadius() const;

    /** Returns the number of vertexes per band (2 total) in the 2D (map view) geometry. */
    int getNPointsPerBandIn2DGeometry() const;

    //@{
    /** Arrays containing the (x,y) vertex coordinates of the two-band map view (2D) geometry. */
    const double* get2DBand1Xs() const { return _2DBand1x; }
    const double* get2DBand1Ys() const { return _2DBand1y; }
    const double* get2DBand2Xs() const { return _2DBand2x; }
    const double* get2DBand2Ys() const { return _2DBand2y; }
    //@}

    /** Returns the distance between the axis along _azimuth and the location given by (x,y). */
    double distanceToAxis( double x, double y );

    /** Returns a polygon of a domain of influence around a center frequency given a frequency span around it.
     * The domain is normally a rectangular area centered at the x,y location in the 2D spectrogram corresponding
     * to the centerFrequency, oriented towards _azimuth, with _bandWidth wide and frequencySpan long.
     * The _azimuthTolerance setting may result in clipping the rectanglar domain.
     */
    QList<QPointF> getAreaOfInfluence( double centerFrequency, double frequencySpan );

public Q_SLOTS:
    void setRadius(double radius);
    void setEndRadius(double endRadius);

protected:
    double _radius;
    double _endRadius;

// ExperimentalVariogramParameters interface
    virtual void updateGeometry();

    //@{
    /** The two-band map view (2D) geometry. */
    static const int _n2DBandPoints = 6;
    double _2DBand1x[_n2DBandPoints];
    double _2DBand1y[_n2DBandPoints];
    double _2DBand2x[_n2DBandPoints];
    double _2DBand2y[_n2DBandPoints];
    SpatialLocation _aPointOnAxis; //a point located on the axis along _azimuth (useful for the distanceToAxis() function).
    //@}
};

#endif // SPECTROGRAM1DPARAMETERS_H
