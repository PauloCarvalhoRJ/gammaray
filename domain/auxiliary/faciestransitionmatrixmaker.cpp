#include "faciestransitionmatrixmaker.h"
#include "domain/segmentset.h"

#include <limits>


//-------------------specializations of the getTrajectoryLength() template function---------------//
namespace FTMMakerAdapters {
    template <>
    double getTrajectoryLength<SegmentSet>( SegmentSet* dataFile ){
        double result = 0.0;
        for( int i = 0; i < dataFile->getDataLineCount(); ++i ){
            result += dataFile->getSegmentLenght( i ) + dataFile->getDistanceToNextSegment( i );
        }
        return result;
    }
}
//------------------------------------------------------------------------------------//


//-------------------specializations of the getAssociatedCategoryDefinition() template function---------------//
namespace FTMMakerAdapters {
    template <>
    CategoryDefinition* getAssociatedCategoryDefinition<SegmentSet>( SegmentSet* dataFile, int variableIndex ){
        Attribute* at = dataFile->getAttributeFromGEOEASIndex( variableIndex+1 );
        return dataFile->getCategoryDefinition( at );
    }
}
//------------------------------------------------------------------------------------//

template <typename Klass> double getValueInTrajectory( Klass* dataFile, int variableIndex,
                                                       double distance, double tolerance );
//-------------------specializations of the getValueInTrajectory() template function---------------//
namespace FTMMakerAdapters {
    template <>
    double getValueInTrajectory<SegmentSet>( SegmentSet* dataFile, int variableIndex,
                                             double distance, double tolerance ){
        //keep track of distance traversed in the trajectory
        double distanceBeforeCurrentSegment = 0.0;
        //for each segment
        for( int i = 0; i < dataFile->getDataLineCount(); ++i ){
            //get segment length
            double segmentLength = dataFile->getSegmentLenght( i );
            //if queried distance (plus or minus tolerance) falls within the segment
            if( (distance - tolerance) <= distanceBeforeCurrentSegment + segmentLength ||
                (distance + tolerance) >= distanceBeforeCurrentSegment )
                //returns the value assigned to the segment.
                return dataFile->data( i, variableIndex );
            //keep track of distance traversed
            distanceBeforeCurrentSegment += segmentLength + dataFile->getDistanceToNextSegment( i );
        }
        return std::numeric_limits<double>::quiet_NaN();
    }
}
//------------------------------------------------------------------------------------//
