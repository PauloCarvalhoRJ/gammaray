#ifndef FACIESTRANSITIONMATRIXMAKER_H
#define FACIESTRANSITIONMATRIXMAKER_H

#include "domain/faciestransitionmatrix.h"
#include "domain/categorydefinition.h"
#include <cassert>

/** Adapters for the different data files.
 *  See the avaliable specializations in the cpp source file.  Add more as needed for
 *  object types other than those already implemented.*/
namespace FTMMakerAdapters {

    /** Returns the total length of the trajectory. */
    template <typename Klass> double getTrajectoryLength( Klass* dataFile );

    /** Returns the value at given distance in trajectory.  Zero corresponds to the
     * beginning of the trajectory and the distance equalling getTrajectoryLength() corresponds
     * to the end of the trajectory.  Negative distances, distances beyond getTrajectoryLength() or
     * distances falling on gaps will cause this function to return std::numeric_limits::quiet_NaN().
     */
    template <typename Klass> double getValueInTrajectory( Klass* dataFile, int variableIndex,
                                                           double distance, double tolerance );

    /** Returns the CategoryDefinition object associated with the given variable.
     * If the variable is not categorical, it returns nullptr.
     */
    template <typename Klass> CategoryDefinition* getAssociatedCategoryDefinition( Klass* dataFile, int variableIndex );
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
     * begins from the end (early in geologic time) to the start (late in geologic time) of the sequence.
     *
     * @param h         Separation in length units.
     * @param tolerance Tolerance in length units.  Normally this is useful when values are
     *                  located in points (zero size).
     */
    FaciesTransitionMatrix makeAlongTrajectory( double h, double tolerance ){
        //retrieve category definition
        CategoryDefinition* cd = FTMMakerAdapters::getAssociatedCategoryDefinition( m_dataFileWithFacies, m_variableIndex );
        assert( cd && "FaciesTransitionMatrixMaker::makeAlongTrajectory(): null CategoryDefinition." );
        //create an empty FTM
        FaciesTransitionMatrix ftm("");
        ftm.setInfo( cd->getName() );
        ftm.initialize();
        //traverse trajectory in steps of size h counting facies transitions
        //from end (early in geologic time) to begining (late in geologic time).
        double trajectoryLength = FTMMakerAdapters::getTrajectoryLength( m_dataFileWithFacies );
        int previousFaciesCode = -9999; //hopefully no one uses -9999 as code.
        for( double distance = trajectoryLength; distance >= 0.0; distance -= h ){
            int faciesCode = -9998;
            double readValue = FTMMakerAdapters::getValueInTrajectory
                               ( m_dataFileWithFacies, m_variableIndex, distance, tolerance );
            if( std::isfinite( readValue ) ){
                faciesCode = static_cast<int>( readValue );
                if( faciesCode != previousFaciesCode && previousFaciesCode != -9999 ){
                    //std::cout << "zzzzzzzzzzzzzzzzz" << std::endl;
                    ftm.incrementCount( previousFaciesCode, faciesCode );
                }
                previousFaciesCode = faciesCode;
            }
        }
        return ftm;
    }

private:
    Klass* m_dataFileWithFacies;
    int m_variableIndex;
};

#endif // FACIESTRANSITIONMATRIXMAKER_H
