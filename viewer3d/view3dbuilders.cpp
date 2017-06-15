#include "view3dbuilders.h"
#include "domain/application.h"
#include "domain/projectcomponent.h"
#include "domain/pointset.h"
#include "domain/attribute.h"
#include "domain/cartesiangrid.h"
#include "view3dcolortables.h"

#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkLookupTable.h>
#include <vtkPlaneSource.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>

View3DBuilders::View3DBuilders()
{
}

vtkSmartPointer<vtkActor> View3DBuilders::build(ProjectComponent *object)
{
    QString object_locator = object->getObjectLocator();
    QString generic_class = object_locator.split(':')[0];
    QString specific_class = object_locator.split(':')[1];
    Application::instance()->logError("view3DBuilders::build(): graphic builder for objects of type " +
                                      generic_class + ":" + specific_class + ":*"
                                      + " not found.");
    return vtkSmartPointer<vtkActor>::New();
}

vtkSmartPointer<vtkActor> View3DBuilders::build(PointSet *object)
{
    //use a more meaningful name.
    PointSet *pointSet = object;

    // Create the geometry of a point (the coordinate)
    vtkSmartPointer<vtkPoints> points =
      vtkSmartPointer<vtkPoints>::New();
    ///////const float p[3] = {1.0, 2.0, 3.0};

    // Create the topology of the point (a vertex)
    vtkSmartPointer<vtkCellArray> vertices = vtkSmartPointer<vtkCellArray>::New();
    ///////vtkIdType pid[1];
    ///////pid[0] = points->InsertNextPoint(p);
    ///////vertices->InsertNextCell(1,pid);

    //read point geometry
    vtkSmartPointer<vtkIdList> pids = vtkSmartPointer<vtkIdList>::New();
    pointSet->loadData();
    pids->Allocate( pointSet->getDataLineCount() );
    for( uint line = 0; line < pointSet->getDataLineCount(); ++line){
        double x = pointSet->data( line, pointSet->getXindex()-1 );
        double y = pointSet->data( line, pointSet->getYindex()-1 );
        double z = 0.0;
        if( pointSet->is3D() )
            z = pointSet->data( line, pointSet->getZindex()-1 );
        pids->InsertNextId(  points->InsertNextPoint( x, y, z ) );
    }
    vertices->InsertNextCell( pids );

    // Create a polydata object
    vtkSmartPointer<vtkPolyData> pointCloud =
      vtkSmartPointer<vtkPolyData>::New();

    // Set the points and vertices we created as the geometry and topology of the polydata
    pointCloud->SetPoints(points);
    pointCloud->SetVerts(vertices);

    // Visualize
    vtkSmartPointer<vtkPolyDataMapper> mapper =
      vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(pointCloud);

    // Create and configure the actor
    vtkSmartPointer<vtkActor> actor =
      vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetPointSize(3);

    return actor;
}

vtkSmartPointer<vtkActor> View3DBuilders::build(Attribute *object)
{
    //use a more meaningful name.
    Attribute *attribute = object;

    //get the attribute's parent data file
    File *file = attribute->getContainingFile();

    //get the parent file type
    QString fileType = file->getFileType();

    if( fileType == "POINTSET" ){
        return buildForAttributeFromPointSet( (PointSet*)file, attribute );
    } else {
        Application::instance()->logError("View3DBuilders::build(Attribute *): Attribute belongs to unsupported file type: " + fileType);
        return vtkSmartPointer<vtkActor>::New();
    }
}

vtkSmartPointer<vtkActor> View3DBuilders::build(CartesianGrid *object)
{
    //use a more meaningful name.
    CartesianGrid *cartesianGrid = object;

    if( cartesianGrid->getNZ() < 2 ){
        return buildForMapCartesianGrid( cartesianGrid );
    } else {
        Application::instance()->logError("View3DBuilders::build(CartesianGrid *): Cartesian grid of unsupported geometry.");
        return vtkSmartPointer<vtkActor>::New();
    }
}

vtkSmartPointer<vtkActor> View3DBuilders::buildForAttributeFromPointSet(PointSet* pointSet, Attribute *attribute)
{
    // Create the geometry of a point (the coordinate)
    vtkSmartPointer<vtkPoints> points =
      vtkSmartPointer<vtkPoints>::New();

    // Create the topology of the point (a vertex)
    vtkSmartPointer<vtkCellArray> vertices = vtkSmartPointer<vtkCellArray>::New();

    //loads data in file, because it's necessary.
    pointSet->loadData();

    //get the variable index in parent data file
    uint var_index = pointSet->getFieldGEOEASIndex( attribute->getName() );

    //get the max and min of the selected variable
    double min = pointSet->min( var_index-1 );
    double max = pointSet->max( var_index-1 );

    //create a VTK array to store the sample values
    vtkSmartPointer<vtkDoubleArray> values = vtkSmartPointer<vtkDoubleArray>::New();
    values->SetName("values");

    //read point geometry and sample values
    vtkSmartPointer<vtkIdList> pids = vtkSmartPointer<vtkIdList>::New();
    pids->Allocate( pointSet->getDataLineCount() );
    values->Allocate( pointSet->getDataLineCount() );
    for( uint line = 0; line < pointSet->getDataLineCount(); ++line){
        // sample location
        double x = pointSet->data( line, pointSet->getXindex()-1 );
        double y = pointSet->data( line, pointSet->getYindex()-1 );
        double z = 0.0;
        if( pointSet->is3D() )
            z = pointSet->data( line, pointSet->getZindex()-1 );
        pids->InsertNextId(  points->InsertNextPoint( x, y, z ) );
        // sample value
        double value = pointSet->data( line, var_index - 1 );
        values->InsertNextValue( value );
    }
    vertices->InsertNextCell( pids );

    // Create a polydata object (topological object)
    vtkSmartPointer<vtkPolyData> pointCloud =
      vtkSmartPointer<vtkPolyData>::New();

    // Set the points, values and vertices we created as the geometry and topology of the polydata
    pointCloud->SetPoints(points);
    pointCloud->SetVerts(vertices);
    pointCloud->GetPointData()->SetScalars( values );
    pointCloud->GetPointData()->SetActiveScalars("values");

    //assign a color table
    vtkSmartPointer<vtkLookupTable> lut = View3dColorTables::getColorTable( ColorTable::RAINBOW, min, max);

    // Create a visualization parameters object
    vtkSmartPointer<vtkPolyDataMapper> mapper =
      vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(pointCloud);
    mapper->SetLookupTable(lut);
    mapper->SetScalarModeToUsePointFieldData();
    mapper->SetColorModeToMapScalars();
    mapper->SelectColorArray("values");
    mapper->SetScalarRange(min, max);

    // Create and configure the final actor object
    vtkSmartPointer<vtkActor> actor =
      vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetPointSize(3);

    return actor;
}

vtkSmartPointer<vtkActor> View3DBuilders::buildForMapCartesianGrid(CartesianGrid *cartesianGrid)
{

    //get grid geometric parameters
    int nX = cartesianGrid->getNX();
    int nY = cartesianGrid->getNY();
    double X0 = cartesianGrid->getX0();
    double Y0 = cartesianGrid->getY0();
    double dX = cartesianGrid->getDX();
    double dY = cartesianGrid->getDY();
    double azimuth = cartesianGrid->getRot();
    double X0frame = X0 - dX/2.0;
    double Y0frame = Y0 - dY/2.0;

    // set up a transform to apply the rotation about the grid origin (center of the first cell)
    vtkSmartPointer<vtkTransform> xform = vtkSmartPointer<vtkTransform>::New();
    xform->Translate( X0, Y0, 0.0);
    xform->RotateZ( azimuth );
    xform->Translate( -X0, -Y0, 0.0);

    //build a VTK 2D regular grid object based on GSLib grid convention
    vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
    plane->SetXResolution( nX );
    plane->SetYResolution( nY );
    plane->SetOrigin( X0frame, Y0frame, 0.0 );
    plane->SetPoint1( X0 + nX * dX, Y0frame, 0.0);
    plane->SetPoint2( X0frame, Y0 + nY * dY, 0.0);
    plane->Update();

    //apply the transform (rotation) to the plane
    vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter =
            vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transformFilter->SetInputConnection(plane->GetOutputPort());
    transformFilter->SetTransform(xform);
    transformFilter->Update();

    // Create a visualization parameters object
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(transformFilter->GetOutputPort());
    mapper->Update();

    // Finally, create and return the actor
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    return actor;
}
