#ifndef EXPERIMENTALVARIOGRAMPARAMETERS_H
#define EXPERIMENTALVARIOGRAMPARAMETERS_H

#include <QObject>

/** This class represents experimental variogram calculation parameters such as azimuth, bandwidth, etc. */
class ExperimentalVariogramParameters : public QObject
{
    Q_OBJECT

public:

    ExperimentalVariogramParameters();

    double azimuth() const;
    double azimuthTolerance() const;
    double bandWidth() const;

public Q_SLOTS:
    void setAzimuth(double azimuth);
    void setAzimuthTolerance(double azimuthTolerance);
    void setBandWidth(double bandWidth);

protected:
    double _azimuth;
    double _azimuthTolerance;
    double _bandWidth;
};

#endif // EXPERIMENTALVARIOGRAMPARAMETERS_H
