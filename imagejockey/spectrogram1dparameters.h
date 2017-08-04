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

public Q_SLOTS:
    void setRadius(double radius);
    void setEndRadius(double endRadius);

protected:
    /** The radius from the spectrogram center where the band starts outwardly. */
    double _radius;

    /** The radius from the spectrogram center where the band ends. */
    double _endRadius;

};

#endif // SPECTROGRAM1DPARAMETERS_H
