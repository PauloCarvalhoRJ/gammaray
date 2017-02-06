#ifndef UNIVARIATEDISTRIBUTION_H
#define UNIVARIATEDISTRIBUTION_H

#include "distribution.h"

/**
 * @brief The UnivariateDistribution class represents a univariate distribution model, normally
 * computed with the histsmth GSLib program, that was saved by the user in the project directory.
 * Physically, univariate distribution files are formatted as pointset files with variables columns, relating
 * values with frequencies.
 */
class UnivariateDistribution : public Distribution
{
public:
    UnivariateDistribution( const QString path );

// File interface
public:
    QString getFileType(){ return "UNIDIST"; }

// ProjectComponent interface
public:
    QIcon getIcon();
};

#endif // UNIVARIATEDISTRIBUTION_H
