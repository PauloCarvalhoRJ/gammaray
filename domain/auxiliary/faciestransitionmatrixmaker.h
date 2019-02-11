#ifndef FACIESTRANSITIONMATRIXMAKER_H
#define FACIESTRANSITIONMATRIXMAKER_H

#include "domain/faciestransitionmatrix.h"
#include "domain/segmentset.h"


/** Adapters for the different data files.
 *  See the avaliable specializations in the cpp source file.  Add more as needed for
 *  object types other than those already implemented.*/
namespace FTMMakerAdapters {
    /** Returns the total length of the trajectory. */
    template <typename Klass> double getTrajectoryLength( Klass* dataFile );
    /** Returns the value at given distance in trajectory.  Zero corresponds to the
     * beginning of the trajectory and the distance equalling getTrajectoryLength() corresponds
     * to the end of the trajectory.
     */
    template <typename Klass> double getValueInTrajectory( Klass* dataFile );
}


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
     * Computes a facies transition matrix by counting facies changes at the given separation h along
     * the trajectory of the object passed in the constructor.  The object is assumed
     * to be a series of data (e.g. point set) ordered along a linear trajectory
     * (e.g. of a well, drillhole or countour).  Stratigraphy convention requires that the facies count
     * begins from the end (older) to the start (younger) of the sequence.
     *
     * @param h         Separation in length units.
     * @param tolerance Tolerance in length units.  Normally this is useful when values are
     *                  located in points (zero size).
     */
    void makeAlongTrajectory( double h, double tolerance ){
        Q_UNUSED( h );
        Q_UNUSED( tolerance );
        double trajectoryLength = FTMMakerAdapters::getTrajectoryLength( m_dataFileWithFacies );
    }

private:
    Klass* m_dataFileWithFacies;
    int m_variableIndex;
};

#endif // FACIESTRANSITIONMATRIXMAKER_H
