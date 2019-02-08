#ifndef FACIESTRANSITIONMATRIXMAKER_H
#define FACIESTRANSITIONMATRIXMAKER_H

#include "domain/faciestransitionmatrix.h"


/** This is a template class used to make facies transition matrices for different
 *  domain objects and with different parameters.
 */

template <class Klass>
class FaciesTransitionMatrixMaker
{
public:

    FaciesTransitionMatrixMaker( Klass* dataFileWithFacies, int variableIndex ) :
        m_dataFileWithFacies( dataFileWithFacies ),
        m_variableIndex( variableIndex )
    {
    }

    /**
     * Computes a matrix by counting facies changes at the given separation h along
     * the trajectory of the object passed in the constructor.  The object is assumed
     * to be a series of data (e.g. point set) ordered along a linear trajectory
     * (e.g. of a well, drillhole or countour).
     *
     * @param h         Separation in length units.
     * @param tolerance Tolerance in length units.  Normally this is useful when values are
     *                  located in points (zero size).
     */
    FaciesTransitionMatrix makeAlongTrajectory( double h, double tolerance ){
        int countData = getDataCount( m_dataFileWithFacies );
    }

private:
    Klass* m_dataFileWithFacies;
    int m_variableIndex;
};

#endif // FACIESTRANSITIONMATRIXMAKER_H
