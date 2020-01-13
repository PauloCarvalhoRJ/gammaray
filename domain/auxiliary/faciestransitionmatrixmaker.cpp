#include "faciestransitionmatrixmaker.h"
#include "domain/segmentset.h"
#include "domain/application.h"

#include <limits>

//VTK is used here for its geometry algorithms
//#include <vtkSmartPointer.h>
//#include <vtkIntArray.h>
//#include <vtkCellArray.h>
//#include <vtkPoints.h>
//#include <vtkLine.h>
//#include <vtkPolyData.h>
//#include <vtkCellData.h>
//#include <vtkExtractEdges.h>

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

//-------------------specializations of the getValue() template function---------------//
namespace FTMMakerAdapters {
    template <>
    double getValue<DataFile>( DataFile* dataFile, int variableIndex, int dataIndex ){
        return dataFile->data( dataIndex, variableIndex );
    }
}
//------------------------------------------------------------------------------------//


//-------------------specializations of the getFaciesSequence() template function---------------//
namespace FTMMakerAdapters {

    template <>
    std::vector< std::vector< int > > getFaciesSequence<SegmentSet>(
                                     SegmentSet* segmentSet,
                                     int dataColumnWithFaciesCodes,
                                     DataSetOrderForFaciesString dataIndexOrder,
                                     int groupByVariableIndex ){

        std::vector< std::vector< int > > result;

        //load data from filesystem
        segmentSet->loadData();

        //Determine the sorting criteria (column and order) from the data index order specified by the client code/user.
        int sortByColumnIndex = -1;
        SortingOrder ascendingOrDescending = SortingOrder::ASCENDING;
        switch ( dataIndexOrder ) {
        case DataSetOrderForFaciesString::FROM_BOTTOM_TO_TOP:  //User wants to go from bottom (lowest Z values) up (highest Z values), hence...
            sortByColumnIndex = segmentSet->getZindex()-1;     //...sort by Z coordinate...
            ascendingOrDescending = SortingOrder::ASCENDING; //...in ascending order.
            break;
        default:
            Application::instance()->logError("FTMMakerAdapters::getFaciesSequence<SegmentSet>(): only "
                                              "DataIndexOrder::FROM_BOTTOM_TO_TOP is currently supported.");
            return result;
        }

        //the outer vector is the data set split into several data tables
        // (may be just one if data is not grouped).
        std::vector< std::vector< std::vector< double > > > dataGroups;
        if( groupByVariableIndex != -1 )
            dataGroups = segmentSet->getDataGroupedBy( groupByVariableIndex );
        else
            dataGroups.push_back( segmentSet->getDataTable() ); //just one group: the whole data set.

        Application::instance()->logInfo("FTMMakerAdapters::getFaciesSequence<SegmentSet>(): Number of data groups: " + QString::number( dataGroups.size() ));

        //for each data group (may be just one if the user opted to not group data (e.g. by drill hole id)).
        for( std::vector< std::vector< double > >& dataGroup : dataGroups ){
            std::vector< int > faciesSequence;

            //Sort the data group to meet the sequence criterion wanted by the user.
            Util::sortDataFrame( dataGroup, sortByColumnIndex, ascendingOrDescending );


            double previousHeadX = std::numeric_limits<double>::quiet_NaN();
            double previousHeadY = std::numeric_limits<double>::quiet_NaN();
            double previousHeadZ = std::numeric_limits<double>::quiet_NaN();
            double previousTailX = std::numeric_limits<double>::quiet_NaN();
            double previousTailY = std::numeric_limits<double>::quiet_NaN();
            double previousTailZ = std::numeric_limits<double>::quiet_NaN();

            //for each segment in the group
            for( const std::vector< double >& segmentData : dataGroup ){

                // get segment geometry
                double currentHeadX = segmentData[ segmentSet->getXindex()-1 ];
                double currentHeadY = segmentData[ segmentSet->getYindex()-1 ];
                double currentHeadZ = segmentData[ segmentSet->getZindex()-1 ];
                double currentTailX = segmentData[ segmentSet->getXFinalIndex()-1 ];
                double currentTailY = segmentData[ segmentSet->getYFinalIndex()-1 ];
                double currentTailZ = segmentData[ segmentSet->getZFinalIndex()-1 ];

                // get the facies code (may be uninformed)
                double faciesCodeAsDouble = segmentData[ dataColumnWithFaciesCodes ];
                bool isNDV = segmentSet->isNDV( faciesCodeAsDouble );
                int faciesCode = static_cast<int>( faciesCodeAsDouble );

                // if we are in the first (informed) segment
                if( faciesSequence.empty() ){
                    //record the facies of the first informed segment.
                    if( ! isNDV )
                        faciesSequence.push_back( faciesCode );
                }else{ // for the other segments...
                    //if the current segment doesn't connect to the previous...
                    if( ! Util::areConnected(  currentHeadX,  currentHeadY,  currentHeadZ,
                                               currentTailX,  currentTailY,  currentTailZ,
                                              previousHeadX, previousHeadY, previousHeadZ,
                                              previousTailX, previousTailY, previousTailZ ) ){
                        // ...append the previous string to the result to be returned.
                        if( ! faciesSequence.empty() )
                            result.push_back( faciesSequence );
                        // and start a new facies string.
                        faciesSequence = std::vector< int >();
                    }
                    // appends the facies (if informed) of the current segment to the current facies string.
                    if( ! isNDV )
                        faciesSequence.push_back( static_cast<int>(segmentData[ dataColumnWithFaciesCodes ]) );
                }

                // keep track of segment geometry for the next iteration to determine connectivity.
                previousHeadX = currentHeadX;
                previousHeadY = currentHeadY;
                previousHeadZ = currentHeadZ;
                previousTailX = currentTailX;
                previousTailY = currentTailY;
                previousTailZ = currentTailZ;
            }
            if( ! faciesSequence.empty() )
                result.push_back( faciesSequence );
        }

        Application::instance()->logInfo("FTMMakerAdapters::getFaciesSequence<SegmentSet>(): Number of contiguous facies sequences: " + QString::number( result.size() ));



//        //get the array indexes for the xyz coordinates
//        //defining the segments
//        uint x0colIdx = segmentSet->getXindex() - 1;
//        uint y0colIdx = segmentSet->getYindex() - 1;
//        uint z0colIdx = segmentSet->getZindex() - 1;
//        uint x1colIdx = segmentSet->getXFinalIndex() - 1;
//        uint y1colIdx = segmentSet->getYFinalIndex() - 1;
//        uint z1colIdx = segmentSet->getZFinalIndex() - 1;

//        //create a VTK array to store the data indexes values
//        vtkSmartPointer<vtkIntArray> dataIndexes = vtkSmartPointer<vtkIntArray>::New();
//        dataIndexes->SetName("dataIndexes");

//        //build point and segment VTK primitives from data
//        vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
//        vtkSmartPointer<vtkCellArray> segments = vtkSmartPointer<vtkCellArray>::New();
//        for( uint i = 0; i < segmentSet->getDataLineCount(); ++i ){
//            double x0 = segmentSet->data( i, x0colIdx );
//            double y0 = segmentSet->data( i, y0colIdx );
//            double z0 = segmentSet->data( i, z0colIdx );
//            double x1 = segmentSet->data( i, x1colIdx );
//            double y1 = segmentSet->data( i, y1colIdx );
//            double z1 = segmentSet->data( i, z1colIdx );
//            vtkIdType id0 = points->InsertNextPoint( x0, y0, z0 );
//            vtkIdType id1 = points->InsertNextPoint( x1, y1, z1 );
//            vtkSmartPointer<vtkLine> segment = vtkSmartPointer<vtkLine>::New();
//            segment->GetPointIds()->SetId( 0, id0 );
//            segment->GetPointIds()->SetId( 1, id1 );
//            segments->InsertNextCell( segment );
//            // take the opportunity to assign the data index value to the respective segment since
//            // the data reside in the segments and not in the vertexes.
//            dataIndexes->InsertNextValue( i );
//        }

//        // build a VTK polygonal line from the points and segments primitives
//        // each segmens has its respective data index as assigned scalar value.
//        vtkSmartPointer<vtkPolyData> poly = vtkSmartPointer<vtkPolyData>::New();
//        poly->SetPoints( points );
//        poly->SetLines( segments );
//        poly->GetCellData()->SetScalars( dataIndexes );


//        vtkSmartPointer<vtkExtractEdges> lineExtractor = vtkSmartPointer<vtkExtractEdges>::New();
//        lineExtractor->SetInputData( poly );
//        lineExtractor->CreateDefaultLocator();
//        lineExtractor->Update();

//        Application::instance()->logInfo("=====>" + QString::number( poly->GetNumberOfCells() ));
//        Application::instance()->logInfo("=====>" + QString::number( lineExtractor->GetOutput()->GetNumberOfCells() ));

        return result;
    }

    template <>
    std::vector< std::vector< int > > getFaciesSequence<DataFile>(
                                     DataFile* dataFile,
                                     int dataColumnWithFaciesCodes,
                                     DataSetOrderForFaciesString dataIndexOrder,
                                     int groupByVariableIndex ){
        if( dataFile->getFileType() == "SEGMENTSET" ){
            SegmentSet* segmentSet = dynamic_cast<SegmentSet*>( dataFile );
            return getFaciesSequence<SegmentSet>( segmentSet, dataColumnWithFaciesCodes, dataIndexOrder, groupByVariableIndex );
        } else {
            Application::instance()->logError("FTMMakerAdapters::getFaciesSequence<DataFile>(): Unsupported data file type: "
                                              + dataFile->getFileType() + ". Returned empty sequence." );
            return std::vector< std::vector< int > >();
        }
    }
}
//------------------------------------------------------------------------------------//


