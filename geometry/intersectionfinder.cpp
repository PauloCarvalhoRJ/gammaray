#include "intersectionfinder.h"
#include "viewer3d/view3dbuilders.h"
#include "domain/cartesiangrid.h"
#include "domain/application.h"
#include "domain/segmentset.h"

#include <vtkSmartPointer.h>
#include <vtkOBBTree.h>
#include <vtkUnstructuredGrid.h>
#include <vtkGeometryFilter.h>

IntersectionFinder::IntersectionFinder() :
    m_tree( nullptr )
{

}

void IntersectionFinder::initWithSurface( CartesianGrid *cg, int variableIndexOfZ )
{
    // Get the variable with the z values.
    Attribute* attributeWithZvalues = cg->getAttributeFromGEOEASIndex( variableIndexOfZ + 1 );

    // Make a surface object from the data.
    vtkSmartPointer< vtkUnstructuredGrid > surface =
            View3DBuilders::makeSurfaceFrom2DGridWithZvalues( cg, attributeWithZvalues );

    // Sanity check.
    if( ! surface ){
        Application::instance()->logError( "IntersectionFinder::initWithSurface(): null surface."
                                           " Check console for error messages." );
        return;
    }

    // Extract the geometry (vtkPolyData) from the grid as required by vtkOBBTree API.
    vtkSmartPointer<vtkGeometryFilter> geometryFilter =  vtkSmartPointer<vtkGeometryFilter>::New();
    geometryFilter->SetInputData( surface );
    geometryFilter->Update();
    vtkSmartPointer<vtkPolyData> polydata = geometryFilter->GetOutput();

    // Sanity check.
    if( ! polydata->GetNumberOfPoints() ){
        Application::instance()->logError( "IntersectionFinder::initWithSurface(): extracting the geometry of a "
                                           "vtkUnstructuredGrid resulted in an empty object." );
        return;
    }

    // Create the VTK locator.
    m_tree = vtkSmartPointer<vtkOBBTree>::New();
    m_tree->SetDataSet( polydata );
    m_tree->BuildLocator();
}

std::vector<Vector3D> IntersectionFinder::getIntersections(const SegmentSet *segmentSet)
{
    std::vector<Vector3D> result;

    vtkSmartPointer<vtkPoints> intersectPoints = vtkSmartPointer<vtkPoints>::New();

    if( ! segmentSet->getDataLineCount() ){
        Application::instance()->logError("IntersectionFinder::getIntersections(SegmentSet): No data. "
                                          "Perhaps a prior call to DataFile::loadData() is missing.");
        return std::vector<Vector3D>();
    }

    //for each segment
    for( int iSegment = 0; iSegment < segmentSet->getDataLineCount(); ++iSegment ) {

        //get the segment's vertexes
        double vi[3] = { segmentSet->dataConst( iSegment, segmentSet->getXindex()-1 ),
                         segmentSet->dataConst( iSegment, segmentSet->getYindex()-1 ),
                         segmentSet->dataConst( iSegment, segmentSet->getZindex()-1 ) };
        double vf[3] = { segmentSet->dataConst( iSegment, segmentSet->getXFinalIndex()-1 ),
                         segmentSet->dataConst( iSegment, segmentSet->getYFinalIndex()-1 ),
                         segmentSet->dataConst( iSegment, segmentSet->getZFinalIndex()-1 ) };

        //compute intersection
        intersectPoints->Reset();
        m_tree->IntersectWithLine(vi, vf, intersectPoints, nullptr);

        // get the intersection points (if any)
        double intersection[3];
        for(int i = 0; i < intersectPoints->GetNumberOfPoints(); ++i ) {
            intersectPoints->GetPoint(i, intersection);
            Vector3D intersect;
            intersect.x = intersection[0];
            intersect.y = intersection[1];
            intersect.z = intersection[2];
            result.push_back( intersect );
        }
    }

    return result;
}

std::vector<Vector3D> IntersectionFinder::getIntersectionsConnectingGaps(const SegmentSet *segmentSet)
{
    std::vector<Vector3D> result;

    vtkSmartPointer<vtkPoints> intersectPoints = vtkSmartPointer<vtkPoints>::New();

    if( ! segmentSet->getDataLineCount() ){
        Application::instance()->logError("IntersectionFinder::getIntersectionsConnectingGaps(SegmentSet): No data. "
                                          "Perhaps a prior call to DataFile::loadData() is missing.");
        return std::vector<Vector3D>();
    }

    //for each segment
    for( int iSegment = 0; iSegment < segmentSet->getDataLineCount(); ++iSegment ) {

        //get the segment's vertexes
        double vi[3] = { segmentSet->dataConst( iSegment, segmentSet->getXindex()-1 ),
                         segmentSet->dataConst( iSegment, segmentSet->getYindex()-1 ),
                         segmentSet->dataConst( iSegment, segmentSet->getZindex()-1 ) };
        double vf[3] = { segmentSet->dataConst( iSegment, segmentSet->getXFinalIndex()-1 ),
                         segmentSet->dataConst( iSegment, segmentSet->getYFinalIndex()-1 ),
                         segmentSet->dataConst( iSegment, segmentSet->getZFinalIndex()-1 ) };

        //compute intersection
        intersectPoints->Reset();
        m_tree->IntersectWithLine(vi, vf, intersectPoints, nullptr);

        // get the intersection points (if any)
        double intersection[3];
        for(int i = 0; i < intersectPoints->GetNumberOfPoints(); ++i ) {
            intersectPoints->GetPoint(i, intersection);
            Vector3D intersect;
            intersect.x = intersection[0];
            intersect.y = intersection[1];
            intersect.z = intersection[2];
            result.push_back( intersect );
        }

        //from the 2nd segment and on, consider any gaps as part of the trajectory.
        if( iSegment > 0 ) {

            // If there is a gap between current and previus segments...
            if( ! Util::almostEqual2sComplement( 0.0,
                                                 segmentSet->getDistanceToNextSegmentConst( iSegment-1 ),
                                                 1 ) ) {

                //get the line starting at the tail vertex of the previous segment and the head vertex
                //of the current segment.
                double vi[3] = { segmentSet->dataConst( iSegment-1, segmentSet->getXFinalIndex()-1 ),
                                 segmentSet->dataConst( iSegment-1, segmentSet->getYFinalIndex()-1 ),
                                 segmentSet->dataConst( iSegment-1, segmentSet->getZFinalIndex()-1 ) };
                double vf[3] = { segmentSet->dataConst( iSegment,   segmentSet->getXindex()-1 ),
                                 segmentSet->dataConst( iSegment,   segmentSet->getYindex()-1 ),
                                 segmentSet->dataConst( iSegment,   segmentSet->getZindex()-1 ) };

                //compute intersection
                intersectPoints->Reset();
                m_tree->IntersectWithLine(vi, vf, intersectPoints, nullptr);

                // get the intersection points (if any)
                double intersection[3];
                for(int i = 0; i < intersectPoints->GetNumberOfPoints(); ++i ) {
                    intersectPoints->GetPoint(i, intersection);
                    Vector3D intersect;
                    intersect.x = intersection[0];
                    intersect.y = intersection[1];
                    intersect.z = intersection[2];
                    result.push_back( intersect );
                }
            }
        }
    }

    return result;
}
