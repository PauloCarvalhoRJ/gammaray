#include "thicknesscalculator.h"
#include "domain/segmentset.h"


//-------------------specializations of the getNumberOfIntervalUnits() template function---------------//
namespace ThicknessCalculatorAdapters {
    template <>
    int getNumberOfIntervalUnits<SegmentSet>( SegmentSet* dataFile ){
        return dataFile->getDataLineCount();
    }
}
//------------------------------------------------------------------------------------//

//-------------------specializations of the getThickness() template function---------------//
namespace ThicknessCalculatorAdapters {
    template <>
    double getThickness<SegmentSet>( SegmentSet* dataFile, int intervalUnitIndex ){
        //TODO: this computation does not take into account the dip/strike of the strata
        //      nor the dip/strike of the segment itself.
        return dataFile->getSegmentLenght( intervalUnitIndex );
    }
}
//------------------------------------------------------------------------------------//


//-------------------specializations of the getThickness() template function---------------//
namespace ThicknessCalculatorAdapters {
    template <>
    double getValueInIntervalUnit<SegmentSet>( SegmentSet* dataFile,
                                               int variableIndex,
                                               int intervalUnitIndex ){
        return dataFile->data( intervalUnitIndex, variableIndex );
    }
}
//------------------------------------------------------------------------------------//
