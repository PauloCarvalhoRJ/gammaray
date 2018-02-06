#ifndef IJEXPERIMENTALVARIOGRAMPARAMETERS_H
#define IJEXPERIMENTALVARIOGRAMPARAMETERS_H

#include <QObject>
#include "geostats/spatiallocation.h"

/** This class represents experimental variogram calculation parameters such as azimuth, bandwidth, etc. */
class IJExperimentalVariogramParameters : public QObject
{
    Q_OBJECT

public:

    IJExperimentalVariogramParameters();

    double azimuth() const;
    double azimuthTolerance() const;
    double bandWidth() const;
    /** A spatial location needed to visualize this object. It is not needed for variogram calculation.*/
    const SpatialLocation& refCenter() const;

Q_SIGNALS:
    void updated();

public Q_SLOTS:
    void setAzimuth(double azimuth);
    void setAzimuthTolerance(double azimuthTolerance);
    void setBandWidth(double bandWidth);
    void setRefCenter(const SpatialLocation &refCenter);

protected:
    double _azimuth;
    double _azimuthTolerance;
    double _bandWidth;
    SpatialLocation _refCenter;
    /** Geometry is the set of geometric primitives that results from the parameters. */
    virtual void updateGeometry();
};

#endif // IJEXPERIMENTALVARIOGRAMPARAMETERS_H
