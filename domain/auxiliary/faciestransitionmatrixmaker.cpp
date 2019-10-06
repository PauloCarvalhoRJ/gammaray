#include "faciestransitionmatrixmaker.h"
#include "domain/segmentset.h"
#include "domain/application.h"

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
//            std::cout << "distance = " << distance << "; tolerance = " << tolerance
//                      << "; distancet = " << distanceBeforeCurrentSegment << "; segL = " << segmentLength << std::endl;
            if(     ((distance - tolerance) <= distanceBeforeCurrentSegment + segmentLength
                    &&
                    (distance - tolerance) >= distanceBeforeCurrentSegment)
                 ||
                    ((distance + tolerance) <= distanceBeforeCurrentSegment + segmentLength
                    &&
                    (distance + tolerance) >= distanceBeforeCurrentSegment)
              ){
                //returns the value assigned to the segment.
//                std::cout << dataFile->data( i, variableIndex ) << std::endl;
                return dataFile->data( i, variableIndex );
            }
            //keep track of distance traversed
            distanceBeforeCurrentSegment += segmentLength + dataFile->getDistanceToNextSegment( i );
        }
        if( dataFile->getDataLineCount() == 0 )
            Application::instance()->logWarn( "FTMMakerAdapters::getValueInTrajectory<SegmentSet>(): No data.  Didn't you forget to load the file before?" );
        return std::numeric_limits<double>::quiet_NaN();
    }
}
//------------------------------------------------------------------------------------//
