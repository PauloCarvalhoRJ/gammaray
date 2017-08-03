#ifndef SPECTROGRAM1DPARAMETERS_H
#define SPECTROGRAM1DPARAMETERS_H

#include "geostats/experimentalvariogramparameters.h"

/** This class contains parameters to calculate a 1D spectrogram from a band in a 2D spectrogram. */
class Spectrogram1DParameters : public ExperimentalVariogramParameters
{
public:
    Spectrogram1DParameters();

    double radius() const;

public Q_SLOTS:
    void setRadius(double radius);

protected:
    /** The radius from the spectrogram center where the band starts outwardly. */
    double _radius;
};

#endif // SPECTROGRAM1DPARAMETERS_H
