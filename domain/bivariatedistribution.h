#ifndef BIVARIATEDISTRIBUTION_H
#define BIVARIATEDISTRIBUTION_H

#include "distribution.h"

/**
 * @brief The BivariateDistribution class represents a bivariate distribution model, normally
 * computed with the scatsmth GSLib program, that was saved by the user in the project directory.
 * Physically, bivariate distribution files are formatted as cartesian grid files with variables columns, relating
 * X and Y values with frequencies.
 */
class BivariateDistribution : public Distribution
{
public:
    BivariateDistribution( const QString path );

// File interface
public:
    QString getFileType(){ return "BIDIST"; }

// ProjectComponent interface
public:
    QIcon getIcon();
};


#endif // BIVARIATEDISTRIBUTION_H
