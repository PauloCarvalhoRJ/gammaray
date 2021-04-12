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

//-------------------specializations of the populateSpatialIndex() template function---------------//
namespace VPCMakerAdapters {
    template <>
    void populateSpatialIndex<SegmentSet>( SegmentSet* dataFile, SpatialIndex& spatialIndex ){
        spatialIndex.fill( dataFile, 0.000001 );
    }
}
//------------------------------------------------------------------------------------//

//-------------------specializations of the getValue() template function---------------//
namespace VPCMakerAdapters {
    template <>
    double getValue<DataFile>( DataFile* dataFile, int variableIndex, int dataIndex ){
        return dataFile->data( dataIndex, variableIndex );
    }
    template <>
    double getValue<SegmentSet>( SegmentSet* dataFile, int variableIndex, int dataIndex ){
        return getValue<DataFile>( dataFile, variableIndex, dataIndex );
    }
}
//------------------------------------------------------------------------------------//


//-------------------specializations of the populateSpatialIndex() template function---------------//
namespace VPCMakerAdapters {
    template <>
    double getSupportSize<SegmentSet>( SegmentSet* dataFile, int dataIndex ){
        return dataFile->getSegmentLenght( dataIndex );
    }
}
//------------------------------------------------------------------------------------//

//-------------------specializations of the getName() template function---------------//
namespace VPCMakerAdapters {
    template <>
    QString getName<SegmentSet>( SegmentSet* dataFile ){
        return dataFile->getName();
    }
}
//------------------------------------------------------------------------------------//
