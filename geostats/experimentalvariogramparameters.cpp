#include "experimentalvariogramparameters.h"

ExperimentalVariogramParameters::ExperimentalVariogramParameters() : QObject()
{
}
double ExperimentalVariogramParameters::azimuth() const
{
    return _azimuth;
}

void ExperimentalVariogramParameters::setAzimuth(double azimuth)
{
    _azimuth = azimuth;
    emit updated();
}
double ExperimentalVariogramParameters::azimuthTolerance() const
{
    return _azimuthTolerance;
}

void ExperimentalVariogramParameters::setAzimuthTolerance(double azimuthTolerance)
{
    _azimuthTolerance = azimuthTolerance;
    emit updated();
}
double ExperimentalVariogramParameters::bandWidth() const
{
    return _bandWidth;
}

void ExperimentalVariogramParameters::setBandWidth(double bandWidth)
{
    _bandWidth = bandWidth;
    emit updated();
}
SpatialLocation ExperimentalVariogramParameters::refCenter() const
{
    return _refCenter;
}

void ExperimentalVariogramParameters::setRefCenter(const SpatialLocation &refCenter)
{
    _refCenter = refCenter;
}




