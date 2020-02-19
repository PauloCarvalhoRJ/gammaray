#ifndef UNIVARIATEDISTRIBUTION_H
#define UNIVARIATEDISTRIBUTION_H

#include "distribution.h"
#include "domain/pointset.h"

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

    /**
     * Returns a value given a cumulative frequency.
     * The passed cumulative frequency is normally drawn from a random number generator.
     * This method is typically called from value drawing code in simulation algorithms.
     * Returns NaN if somehow the method fails to find the category code corresponding to passed
     * cumulative frequency.
     */
    double getValueFromCumulativeFrequency( double cumulativeProbability ) const;

// File interface
public:
    QString getFileType(){ return "UNIDIST"; }
    bool isDataFile(){ return false; }
	bool isDistribution(){ return true; }
    virtual void readFromFS();

// ProjectComponent interface
public:
    QIcon getIcon();

protected:
    /** PointSet object used as convenient way to manage the distribution's data. */
    PointSet m_data;
};

#endif // UNIVARIATEDISTRIBUTION_H
