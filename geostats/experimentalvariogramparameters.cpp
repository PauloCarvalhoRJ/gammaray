#include "experimentalvariogramparameters.h"
#include "domain/application.h"

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
    updateGeometry();
    emit updated();
}
double ExperimentalVariogramParameters::azimuthTolerance() const
{
    return _azimuthTolerance;
}

void ExperimentalVariogramParameters::setAzimuthTolerance(double azimuthTolerance)
{
    _azimuthTolerance = azimuthTolerance;
    updateGeometry();
    emit updated();
}
double ExperimentalVariogramParameters::bandWidth() const
{
    return _bandWidth;
}

void ExperimentalVariogramParameters::setBandWidth(double bandWidth)
{
    _bandWidth = bandWidth;
    updateGeometry();
    emit updated();
}
const SpatialLocation &ExperimentalVariogramParameters::refCenter() const
{
    return _refCenter;
}

void ExperimentalVariogramParameters::setRefCenter(const SpatialLocation &refCenter)
{
    _refCenter = refCenter;
    updateGeometry();
    emit updated();
}

void ExperimentalVariogramParameters::updateGeometry()
{
    Application::instance()->logError("ExperimentalVariogramParameters::updateGeometry(): not implemented.  No geometry.");
}




