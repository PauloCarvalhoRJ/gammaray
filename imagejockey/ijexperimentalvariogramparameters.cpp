#include "ijexperimentalvariogramparameters.h"
#include "domain/application.h"

IJExperimentalVariogramParameters::IJExperimentalVariogramParameters() : QObject()
{
}
double IJExperimentalVariogramParameters::azimuth() const
{
    return _azimuth;
}

void IJExperimentalVariogramParameters::setAzimuth(double azimuth)
{
    _azimuth = azimuth;
    updateGeometry();
    emit updated();
}
double IJExperimentalVariogramParameters::azimuthTolerance() const
{
    return _azimuthTolerance;
}

void IJExperimentalVariogramParameters::setAzimuthTolerance(double azimuthTolerance)
{
    _azimuthTolerance = azimuthTolerance;
    updateGeometry();
    emit updated();
}
double IJExperimentalVariogramParameters::bandWidth() const
{
    return _bandWidth;
}

void IJExperimentalVariogramParameters::setBandWidth(double bandWidth)
{
    _bandWidth = bandWidth;
    updateGeometry();
    emit updated();
}
const SpatialLocation &IJExperimentalVariogramParameters::refCenter() const
{
    return _refCenter;
}

void IJExperimentalVariogramParameters::setRefCenter(const SpatialLocation &refCenter)
{
    _refCenter = refCenter;
    updateGeometry();
    emit updated();
}

void IJExperimentalVariogramParameters::updateGeometry()
{
    Application::instance()->logError("IJExperimentalVariogramParameters::updateGeometry(): not implemented.  No geometry.");
}




