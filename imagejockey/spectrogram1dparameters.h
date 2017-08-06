#ifndef SPECTROGRAM1DPARAMETERS_H
#define SPECTROGRAM1DPARAMETERS_H

#include "geostats/experimentalvariogramparameters.h"

/** This class contains parameters to calculate a 1D spectrogram from a band in a 2D spectrogram. */
class Spectrogram1DParameters : public ExperimentalVariogramParameters
{

    Q_OBJECT

public:
    Spectrogram1DParameters();

    double radius() const;

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

public Q_SLOTS:
    void setRadius(double radius);
    void setEndRadius(double endRadius);

protected:
    /** The radius from the spectrogram center where the band starts outwardly. */
    double _radius;

    /** The radius from the spectrogram center where the band ends. */
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
    //@}
};

#endif // SPECTROGRAM1DPARAMETERS_H
