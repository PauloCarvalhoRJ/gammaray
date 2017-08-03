#include "spectrogram1dparameters.h"

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
}

