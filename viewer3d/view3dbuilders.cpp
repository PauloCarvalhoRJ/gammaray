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
#include <vtkCellData.h>
#include <vtkImageGridSource.h>
#include <vtkImageCast.h>
#include <vtkImageMapper3D.h>
#include <vtkStructuredGrid.h>
#include <vtkDataSetMapper.h>
#include <vtkShrinkFilter.h>
#include <vtkTransformFilter.h>
#include <vtkStructuredGridClip.h>
#include <vtkPlane.h>

View3DBuilders::View3DBuilders()
{
}

View3DViewData View3DBuilders::build(ProjectComponent *object)
{
    Application::instance()->logError("view3DBuilders::build(): graphic builder for objects of type \"" +
                                      object->getTypeName()
                                      + "\" not found.");
    return View3DViewData();
}

View3DViewData View3DBuilders::build(PointSet *object)
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

    return View3DViewData( actor );
}

View3DViewData View3DBuilders::build(Attribute *object)
{
    //use a more meaningful name.
    Attribute *attribute = object;

    //get the attribute's parent data file
    File *file = attribute->getContainingFile();

    //get the parent file type
    QString fileType = file->getFileType();

    if( fileType == "POINTSET" ){
        return buildForAttributeFromPointSet( (PointSet*)file, attribute );
    } else if( fileType == "CARTESIANGRID" ) {
        CartesianGrid* cg = (CartesianGrid*)file;
        if( cg->getNZ() < 2 ){
            return buildForAttributeInMapCartesianGrid( cg, attribute );
        } else {
            return buildForAttribute3DCartesianGridWithIJKClipping( cg, attribute);
        }
    } else {
        Application::instance()->logError("View3DBuilders::build(Attribute *): Attribute belongs to unsupported file type: " + fileType);
        return View3DViewData();
    }
}

View3DViewData View3DBuilders::build(CartesianGrid *object)
{
    //use a more meaningful name.
    CartesianGrid *cartesianGrid = object;

    if( cartesianGrid->getNZ() < 2 ){
        return buildForMapCartesianGrid( cartesianGrid );
    } else {
        return buildFor3DCartesianGrid( cartesianGrid );
    }
}

View3DViewData View3DBuilders::buildForAttributeFromPointSet(PointSet* pointSet, Attribute *attribute)
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

    return View3DViewData(actor);
}

View3DViewData View3DBuilders::buildForMapCartesianGrid(CartesianGrid *cartesianGrid)
{

    //get grid geometric parameters (loading data is not necessary)
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
    xform->RotateZ( -azimuth );
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
    return View3DViewData(actor);
}

View3DViewData View3DBuilders::buildForAttributeInMapCartesianGrid(CartesianGrid *cartesianGrid, Attribute *attribute)
{
    //load grid data
    cartesianGrid->loadData();

    //get the variable index in parent data file
    uint var_index = cartesianGrid->getFieldGEOEASIndex( attribute->getName() );

    //get the max and min of the selected variable
    double min = cartesianGrid->min( var_index-1 );
    double max = cartesianGrid->max( var_index-1 );

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

    //create a VTK array to store the sample values
    vtkSmartPointer<vtkDoubleArray> values = vtkSmartPointer<vtkDoubleArray>::New();
    values->SetName("values");

    //read sample values
    values->Allocate( nX*nY );
    for( int i = 0; i < nX*nY; ++i){
        // sample value
        double value = cartesianGrid->data( i, var_index - 1 );
        values->InsertNextValue( value );
    }

    //we don't need file's data anymore
    cartesianGrid->freeLoadedData();

    // set up a transform to apply the rotation about the grid origin (center of the first cell)
    vtkSmartPointer<vtkTransform> xform = vtkSmartPointer<vtkTransform>::New();
    xform->Translate( X0, Y0, 0.0);
    xform->RotateZ( -azimuth );
    xform->Translate( -X0, -Y0, 0.0);

    //build a VTK 2D regular grid object based on GSLib grid convention
    vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
    plane->SetXResolution( nX );
    plane->SetYResolution( nY );
    plane->SetOrigin( X0frame, Y0frame, 0.0 );
    plane->SetPoint1( X0 + nX * dX, Y0frame, 0.0);
    plane->SetPoint2( X0frame, Y0 + nY * dY, 0.0);
    plane->Update();

    //assign the grid values to the grid cells
    plane->GetOutput()->GetCellData()->SetScalars( values );

    //apply the transform (rotation) to the plane
    vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter =
            vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transformFilter->SetInputConnection(plane->GetOutputPort());
    transformFilter->SetTransform(xform);
    transformFilter->Update();

    //assign a color table
    vtkSmartPointer<vtkLookupTable> lut = View3dColorTables::getColorTable( ColorTable::RAINBOW, min, max);

    // Create a visualization parameters object
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(transformFilter->GetOutputPort());
    mapper->SetLookupTable(lut);
    mapper->SetScalarRange(min, max);
    mapper->Update();

    // Finally, create and return the actor
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    return View3DViewData(actor);
}

View3DViewData View3DBuilders::buildFor3DCartesianGrid(CartesianGrid *cartesianGrid)
{
    //get grid geometric parameters (loading data is not necessary)
    int nX = cartesianGrid->getNX();
    int nY = cartesianGrid->getNY();
    int nZ = cartesianGrid->getNZ();
    double X0 = cartesianGrid->getX0();
    double Y0 = cartesianGrid->getY0();
    double Z0 = cartesianGrid->getZ0();
    double dX = cartesianGrid->getDX();
    double dY = cartesianGrid->getDY();
    double dZ = cartesianGrid->getDZ();
    double azimuth = cartesianGrid->getRot();
    double X0frame = X0 - dX/2.0;
    double Y0frame = Y0 - dY/2.0;
    double Z0frame = Z0 - dZ/2.0;

    // set up a transform to apply the rotation about the grid origin (location of the first data point)
    vtkSmartPointer<vtkTransform> xform = vtkSmartPointer<vtkTransform>::New();
    xform->Translate( X0, Y0, Z0);
    xform->RotateZ( -azimuth );
    xform->Translate( -X0, -Y0, -Z0);

    // Create a grid (corner-point, explicit geometry)
    //  As GSLib grids are cell-centered, then we must add an extra point in each direction
    vtkSmartPointer<vtkStructuredGrid> structuredGrid =
            vtkSmartPointer<vtkStructuredGrid>::New();
    vtkSmartPointer<vtkPoints> points =
            vtkSmartPointer<vtkPoints>::New();
    for(int k = 0; k <= nZ; ++k)
        for(int j = 0; j <= nY; ++j)
            for(int i = 0; i <= nX; ++i)
                points->InsertNextPoint( X0frame + i * dX,
                                         Y0frame + j * dY,
                                         Z0frame + k * dZ );
    structuredGrid->SetDimensions( nX+1, nY+1, nZ+1 );
    structuredGrid->SetPoints(points);

    //apply the transform (rotation) to the grid
    vtkSmartPointer<vtkTransformFilter> transformFilter =
            vtkSmartPointer<vtkTransformFilter>::New();
    transformFilter->SetInputData( structuredGrid );
    transformFilter->SetTransform(xform);
    transformFilter->Update();

    // Create mapper (visualization parameters)
    vtkSmartPointer<vtkDataSetMapper> mapper =
            vtkSmartPointer<vtkDataSetMapper>::New();
    mapper->SetInputConnection(transformFilter->GetOutputPort());

    // Finally, create and return the actor
    vtkSmartPointer<vtkActor> actor =
            vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->EdgeVisibilityOn();
    return View3DViewData(actor);
}

View3DViewData View3DBuilders::buildForAttribute3DCartesianGrid(CartesianGrid *cartesianGrid, Attribute *attribute)
{
    //load grid data
    cartesianGrid->loadData();

    //get the variable index in parent data file
    uint var_index = cartesianGrid->getFieldGEOEASIndex( attribute->getName() );

    //get the max and min of the selected variable
    double min = cartesianGrid->min( var_index-1 );
    double max = cartesianGrid->max( var_index-1 );

    //get grid geometric parameters (loading data is not necessary)
    int nX = cartesianGrid->getNX();
    int nY = cartesianGrid->getNY();
    int nZ = cartesianGrid->getNZ();
    double X0 = cartesianGrid->getX0();
    double Y0 = cartesianGrid->getY0();
    double Z0 = cartesianGrid->getZ0();
    double dX = cartesianGrid->getDX();
    double dY = cartesianGrid->getDY();
    double dZ = cartesianGrid->getDZ();
    double azimuth = cartesianGrid->getRot();
    double X0frame = X0 - dX/2.0;
    double Y0frame = Y0 - dY/2.0;
    double Z0frame = Z0 - dZ/2.0;

    //create a VTK array to store the sample values
    vtkSmartPointer<vtkDoubleArray> values = vtkSmartPointer<vtkDoubleArray>::New();
    values->SetName("values");

    //read sample values
    values->Allocate( nX*nY*nZ );
    for( int i = 0; i < nX*nY*nZ; ++i){
        // sample value
        double value = cartesianGrid->data( i, var_index - 1 );
        values->InsertNextValue( value );
    }

    //we don't need file's data anymore
    cartesianGrid->freeLoadedData();

    // set up a transform to apply the rotation about the grid origin (location of the first data point)
    vtkSmartPointer<vtkTransform> xform = vtkSmartPointer<vtkTransform>::New();
    xform->Translate( X0, Y0, Z0);
    xform->RotateZ( -azimuth );
    xform->Translate( -X0, -Y0, -Z0);

    // Create a grid (corner-point, explicit geometry)
    //  As GSLib grids are cell-centered, then we must add an extra point in each direction
    vtkSmartPointer<vtkStructuredGrid> structuredGrid =
            vtkSmartPointer<vtkStructuredGrid>::New();
    vtkSmartPointer<vtkPoints> points =
            vtkSmartPointer<vtkPoints>::New();
    for(int k = 0; k <= nZ; ++k)
        for(int j = 0; j <= nY; ++j)
            for(int i = 0; i <= nX; ++i)
                points->InsertNextPoint( X0frame + i * dX,
                                         Y0frame + j * dY,
                                         Z0frame + k * dZ );
    structuredGrid->SetDimensions( nX+1, nY+1, nZ+1 );
    structuredGrid->SetPoints(points);

    //assign the grid values to the grid cells
    structuredGrid->GetCellData()->SetScalars( values );

    //apply the transform (rotation) to the grid
    vtkSmartPointer<vtkTransformFilter> transformFilter =
            vtkSmartPointer<vtkTransformFilter>::New();
    transformFilter->SetInputData( structuredGrid );
    transformFilter->SetTransform(xform);
    transformFilter->Update();

    //assign a color table
    vtkSmartPointer<vtkLookupTable> lut = View3dColorTables::getColorTable( ColorTable::RAINBOW, min, max);

    // Create mapper (visualization parameters)
    vtkSmartPointer<vtkDataSetMapper> mapper =
            vtkSmartPointer<vtkDataSetMapper>::New();
    mapper->SetInputConnection(transformFilter->GetOutputPort());
    mapper->SetLookupTable(lut);
    mapper->SetScalarRange(min, max);
    mapper->Update();

    // Finally, create and return the actor
    vtkSmartPointer<vtkActor> actor =
            vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->EdgeVisibilityOn();
    return View3DViewData(actor);
}

View3DViewData View3DBuilders::buildForAttribute3DCartesianGridWithIJKClipping(
        CartesianGrid *cartesianGrid, Attribute *attribute)
{
    //load grid data
    cartesianGrid->loadData();

    //get the variable index in parent data file
    uint var_index = cartesianGrid->getFieldGEOEASIndex( attribute->getName() );

    //get the max and min of the selected variable
    double min = cartesianGrid->min( var_index-1 );
    double max = cartesianGrid->max( var_index-1 );

    //get grid geometric parameters (loading data is not necessary)
    int nX = cartesianGrid->getNX();
    int nY = cartesianGrid->getNY();
    int nZ = cartesianGrid->getNZ();
    double X0 = cartesianGrid->getX0();
    double Y0 = cartesianGrid->getY0();
    double Z0 = cartesianGrid->getZ0();
    double dX = cartesianGrid->getDX();
    double dY = cartesianGrid->getDY();
    double dZ = cartesianGrid->getDZ();
    double azimuth = cartesianGrid->getRot();
    double X0frame = X0 - dX/2.0;
    double Y0frame = Y0 - dY/2.0;
    double Z0frame = Z0 - dZ/2.0;

    //create a VTK array to store the sample values
    vtkSmartPointer<vtkDoubleArray> values = vtkSmartPointer<vtkDoubleArray>::New();
    values->SetName("values");

    //read sample values
    values->Allocate( nX*nY*nZ );
    for( int i = 0; i < nX*nY*nZ; ++i){
        // sample value
        double value = cartesianGrid->data( i, var_index - 1 );
        values->InsertNextValue( value );
    }

    //we don't need file's data anymore
    cartesianGrid->freeLoadedData();

    // set up a transform to apply the rotation about the grid origin (location of the first data point)
    vtkSmartPointer<vtkTransform> xform = vtkSmartPointer<vtkTransform>::New();
    xform->Translate( X0, Y0, Z0);
    xform->RotateZ( -azimuth );
    xform->Translate( -X0, -Y0, -Z0);

    // Create a grid (corner-point, explicit geometry)
    //  As GSLib grids are cell-centered, then we must add an extra point in each direction
    vtkSmartPointer<vtkStructuredGrid> structuredGrid =
            vtkSmartPointer<vtkStructuredGrid>::New();
    vtkSmartPointer<vtkPoints> points =
            vtkSmartPointer<vtkPoints>::New();
    for(int k = 0; k <= nZ; ++k)
        for(int j = 0; j <= nY; ++j)
            for(int i = 0; i <= nX; ++i)
                points->InsertNextPoint( X0frame + i * dX,
                                         Y0frame + j * dY,
                                         Z0frame + k * dZ );
    structuredGrid->SetDimensions( nX+1, nY+1, nZ+1 );
    structuredGrid->SetPoints(points);

    //assign the grid values to the grid cells
    structuredGrid->GetCellData()->SetScalars( values );

    //apply the transform (rotation) to the grid
    vtkSmartPointer<vtkTransformFilter> transformFilter =
            vtkSmartPointer<vtkTransformFilter>::New();
    transformFilter->SetInputData( structuredGrid );
    transformFilter->SetTransform(xform);
    transformFilter->Update();

    // Setup a clipping object on the transformed grid,
    // initializing it to not clip anything
    vtkSmartPointer<vtkStructuredGridClip> clipper =
            vtkSmartPointer<vtkStructuredGridClip>::New();
    clipper->SetInputConnection(transformFilter->GetOutputPort());
    clipper->ClipDataOn();
    clipper->SetOutputWholeExtent( 0, nX, 0, nY, 0, nZ );

    //assign a color table
    vtkSmartPointer<vtkLookupTable> lut = View3dColorTables::getColorTable( ColorTable::RAINBOW, min, max);

    // Create mapper (visualization parameters)
    vtkSmartPointer<vtkDataSetMapper> mapper =
            vtkSmartPointer<vtkDataSetMapper>::New();
    mapper->SetInputConnection(clipper->GetOutputPort());
    mapper->SetLookupTable(lut);
    mapper->SetScalarRange(min, max);
    mapper->Update();

    // Finally, pass everything to the actor and return it.
    vtkSmartPointer<vtkActor> actor =
            vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->EdgeVisibilityOn();
    return View3DViewData(actor, clipper);
}

View3DViewData View3DBuilders::buildForStratGrid(ProjectComponent */*toBeSpecified*/)
{
    // Create a grid
    vtkSmartPointer<vtkStructuredGrid> structuredGrid = vtkSmartPointer<vtkStructuredGrid>::New();

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    double x, y, z;

    x = 0.0;
    y = 0.0;
    z = 0.0;

    for(unsigned int k = 0; k < 2; k++)
    {
        z += 2.0;
        for(unsigned int j = 0; j < 3; j++)
        {
            y += 1.0;
            for(unsigned int i = 0; i < 2; i++)
            {
                x += .5;
                points->InsertNextPoint(x, y, z);
            }
        }
    }

    // Specify the dimensions of the grid
    structuredGrid->SetDimensions(2,3,2);
    structuredGrid->SetPoints(points);

    // Create a mapper and actor
    vtkSmartPointer<vtkDataSetMapper> mapper =
            vtkSmartPointer<vtkDataSetMapper>::New();
    mapper->SetInputData(structuredGrid);

    vtkSmartPointer<vtkActor> actor =
            vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    return View3DViewData(actor);
}
