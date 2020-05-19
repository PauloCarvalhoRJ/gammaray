#include "verticalproportioncurvemaker.h"

#include "domain/segmentset.h"

//-------------------specializations of the getAssociatedCategoryDefinition() template function---------------//
namespace VPCMakerAdapters {
    template <>
    CategoryDefinition* getAssociatedCategoryDefinition<DataFile>( DataFile* dataFile, int variableIndex ){
        Attribute* at = dataFile->getAttributeFromGEOEASIndex( variableIndex+1 );
        return dataFile->getCategoryDefinition( at );
    }
    template <>
    CategoryDefinition* getAssociatedCategoryDefinition<SegmentSet>( SegmentSet* dataFile, int variableIndex ){
        return getAssociatedCategoryDefinition<DataFile>( dataFile, variableIndex );
    }
}
//------------------------------------------------------------------------------------//
