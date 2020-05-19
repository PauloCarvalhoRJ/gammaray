#ifndef VERTICALPROPORTIONCURVEMAKER_H
#define VERTICALPROPORTIONCURVEMAKER_H

#include "domain/verticalproportioncurve.h"
#include "domain/categorydefinition.h"
#include "domain/categorypdf.h"
#include <cassert>

/** Adapters for the different data files.
 *  See the avaliable specializations in the cpp source file.  Add more as needed for
 *  object types other than those already implemented.*/
namespace VPCMakerAdapters {

    /** Returns the CategoryDefinition object associated with the given variable.
     * If the variable is not categorical, it returns nullptr.
     */
    template <typename Klass> CategoryDefinition* getAssociatedCategoryDefinition( Klass* dataFile,
                                                                                   int variableIndex );

}

/** This is a template class used to make vertical proportion curves for different
 *  domain objects and with different parameters.
 */
template <class Klass>
class VerticalProportionCurveMaker
{

public:
    VerticalProportionCurveMaker( Klass* dataFileWithFacies, int variableIndex ) :
        m_dataFileWithFacies( dataFileWithFacies ),
        m_variableIndex( variableIndex )
    {}

    /**
     * Computes a vertical proportion curve by counting facies between a top and a base depth
     * of the object passed in the constructor.
     *
     * @param resolution  Resolution in fraction between top and base (e.g.: 0.01 for 1%). The resolution
     *                    sets the number of steps between top and base.  E.g.: a 1% resulution means a curve
     *                    with 100 samples.
     * @param window      Computation window in fraction between top and base (e.g.: 0.03 for 3%).
     *                    The window is used to get smoother curves for high-resolution data such as wells.
     * @param top         Initial depth.
     * @param base        Final depth.
     * @param fallBackPDF The fallback PDF sets default proportions when no data is found in a given step.
     */
    VerticalProportionCurve makeInDepthInterval( double resolution,
                                                 double window,
                                                 double top,
                                                 double base,
                                                 const CategoryPDF& fallBackPDF ){
        //retrieve category definition
        CategoryDefinition* cd = VPCMakerAdapters::getAssociatedCategoryDefinition( m_dataFileWithFacies, m_variableIndex );
        assert( cd && "VerticalProportionCurveMaker::makeInDepthInterval(): null CategoryDefinition." );
        //create an empty VPC
        VerticalProportionCurve vpc("", cd->getName() );
        //traverse trajectory in steps of size h counting facies transitions
        //from end (early in geologic time) to begining (late in geologic time).
//        double trajectoryLength = VPCMakerAdapters::getTrajectoryLength( m_dataFileWithFacies );
//        int previousFaciesCode = -9999; //hopefully no one uses -9999 as code.
//        for( double distance = trajectoryLength; distance >= 0.0; distance -= h ){
//            int faciesCode = -9998;
//            double readValue = VPCMakerAdapters::getValueInTrajectory
//                               ( m_dataFileWithFacies, m_variableIndex, distance, tolerance );
//            if( std::isfinite( readValue ) ){
//                faciesCode = static_cast<int>( readValue );
//                if( /*faciesCode != previousFaciesCode &&*/ previousFaciesCode != -9999 ){
//                    ftm.incrementCount( previousFaciesCode, faciesCode );
//                }
//                previousFaciesCode = faciesCode;
//            }
//        }
        return vpc;
    }

private:
    Klass* m_dataFileWithFacies;
    int m_variableIndex;
};

#endif // VERTICALPROPORTIONCURVEMAKER_H
