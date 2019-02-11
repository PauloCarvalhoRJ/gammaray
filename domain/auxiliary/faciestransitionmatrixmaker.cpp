#include "faciestransitionmatrixmaker.h"
//#include "domain/segmentset.h"


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
