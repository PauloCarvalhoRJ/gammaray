#include "view3dbuilders.h"
#include "domain/application.h"
#include "domain/projectcomponent.h"
#include "domain/pointset.h"
#include "domain/attribute.h"
#include "domain/cartesiangrid.h"
#include "view3dcolortables.h"
#include "view3dwidget.h"

#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkImageActor.h>
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
#include <vtkDecimatePro.h>
#include <vtkTriangleFilter.h>
#include <vtkFloatArray.h>
#include <vtkExtractGrid.h>
#include <vtkLODProp3D.h>
#include <vtkRenderer.h>
#include <vtkCallbackCommand.h>
#include <vtkRenderWindow.h>
#include <vtkThreshold.h>
#include <QMessageBox>

void RefreshCallback( vtkObject* vtkNotUsed(caller),
                      long unsigned int vtkNotUsed(eventId),
                      void* clientData,
                      void* vtkNotUsed(callData) )
{
  vtkSmartPointer<vtkLODProp3D> lodProp =
    static_cast<vtkLODProp3D*>(clientData);
  //Application::instance()->logInfo( "Last rendered LOD ID: " + QString::number( lodProp->GetLastRenderedLODID() ) );
}

View3DBuilders::View3DBuilders()
{
}

View3DViewData View3DBuilders::build(ProjectComponent *object, View3DWidget */*widget3D*/)
{
    Application::instance()->logError("view3DBuilders::build(): graphic builder for objects of type \"" +
                                      object->getTypeName()
                                      + "\" not found.");
    return View3DViewData();
}

View3DViewData View3DBuilders::build(PointSet *object, View3DWidget */*widget3D*/)
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

View3DViewData View3DBuilders::build(Attribute *object, View3DWidget *widget3D)
{
    //use a more meaningful name.
    Attribute *attribute = object;

    //get the attribute's parent data file
    File *file = attribute->getContainingFile();

    //get the parent file type
    QString fileType = file->getFileType();

    if( fileType == "POINTSET" ){
        return buildForAttributeFromPointSet( (PointSet*)file, attribute, widget3D );
    } else if( fileType == "CARTESIANGRID" ) {
        CartesianGrid* cg = (CartesianGrid*)file;
        if( cg->getNZ() < 2 ){
            return buildForAttributeInMapCartesianGridWithVtkStructuredGrid( cg, attribute, widget3D );
        } else {
            return buildForAttribute3DCartesianGridWithIJKClipping( cg, attribute, widget3D );
        }
    } else {
        Application::instance()->logError("View3DBuilders::build(Attribute *): Attribute belongs to unsupported file type: " + fileType);
        return View3DViewData();
    }
}

View3DViewData View3DBuilders::build(CartesianGrid *object, View3DWidget * widget3D )
{
    //use a more meaningful name.
    CartesianGrid *cartesianGrid = object;

    if( cartesianGrid->getNZ() < 2 ){
        return buildForMapCartesianGrid( cartesianGrid, widget3D );
    } else {
        return buildFor3DCartesianGrid( cartesianGrid, widget3D );
    }
}

View3DViewData View3DBuilders::buildForAttributeFromPointSet(PointSet* pointSet,
                                                             Attribute *attribute,
                                                             View3DWidget */*widget3D*/)
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
    vtkSmartPointer<vtkFloatArray> values = vtkSmartPointer<vtkFloatArray>::New();
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

View3DViewData View3DBuilders::buildForMapCartesianGrid(CartesianGrid *cartesianGrid, View3DWidget */*widget3D*/)
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

View3DViewData View3DBuilders::buildForAttributeInMapCartesianGridWithVtkPlaneSource(CartesianGrid *cartesianGrid,
                                                                                     Attribute *attribute,
                                                                                     View3DWidget */*widget3D*/)
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
    vtkSmartPointer<vtkFloatArray> values = vtkSmartPointer<vtkFloatArray>::New();
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

    //build a VTK 2D regular grid (actually a vtkPolyData) object based on GSLib grid convention
    vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
    plane->SetXResolution( nX );
    plane->SetYResolution( nY );
    plane->SetOrigin( X0frame, Y0frame, 0.0 );
    plane->SetPoint1( X0 + nX * dX, Y0frame, 0.0);
    plane->SetPoint2( X0frame, Y0 + nY * dY, 0.0);
    plane->SetOutputPointsPrecision( vtkAlgorithm::SINGLE_PRECISION );
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

View3DViewData View3DBuilders::buildForAttributeInMapCartesianGridWithVtkStructuredGridAndLOD(CartesianGrid *cartesianGrid, Attribute *attribute, View3DWidget *widget3D)
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
    double X0 = cartesianGrid->getX0();
    double Y0 = cartesianGrid->getY0();
    double dX = cartesianGrid->getDX();
    double dY = cartesianGrid->getDY();
    double azimuth = cartesianGrid->getRot();
    double X0frame = X0 - dX/2.0;
    double Y0frame = Y0 - dY/2.0;

    //create a VTK array to store the sample values
    vtkSmartPointer<vtkFloatArray> values = vtkSmartPointer<vtkFloatArray>::New();
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

    // set up a transform to apply the rotation about the grid origin (location of the first data point)
    vtkSmartPointer<vtkTransform> xform = vtkSmartPointer<vtkTransform>::New();
    xform->Translate( X0, Y0, 0);
    xform->RotateZ( -azimuth );
    xform->Translate( -X0, -Y0, 0);

    // Create a grid (corner-point, explicit geometry)
    //  As GSLib grids are cell-centered, then we must add an extra point in each direction
    vtkSmartPointer<vtkStructuredGrid> structuredGrid =
            vtkSmartPointer<vtkStructuredGrid>::New();
    vtkSmartPointer<vtkPoints> points =
            vtkSmartPointer<vtkPoints>::New();
    for(int k = 0; k <= 1; ++k)
        for(int j = 0; j <= nY; ++j)
            for(int i = 0; i <= nX; ++i)
                points->InsertNextPoint( X0frame + i * dX,
                                         Y0frame + j * dY,
                                         0 + k * 1 );
    structuredGrid->SetDimensions( nX+1, nY+1, 1 );
    structuredGrid->SetPoints(points);

    //assign the grid values to the grid cells
    structuredGrid->GetCellData()->SetScalars( values );

    //apply the transform (rotation) to the grid
    vtkSmartPointer<vtkTransformFilter> transformFilter =
            vtkSmartPointer<vtkTransformFilter>::New();
    transformFilter->SetInputData( structuredGrid );
    transformFilter->SetTransform(xform);
    transformFilter->Update();

    //apply several grid downscaling to enable level-of-detail
    vtkSmartPointer<vtkExtractGrid> sg1 = vtkSmartPointer<vtkExtractGrid>::New();
    sg1->SetInputConnection( transformFilter->GetOutputPort()  );
    sg1->SetSampleRate(50,50,1);
    vtkSmartPointer<vtkExtractGrid> sg2 = vtkSmartPointer<vtkExtractGrid>::New();
    sg2->SetInputConnection( transformFilter->GetOutputPort()  );
    sg2->SetSampleRate(25,25,1);
    vtkSmartPointer<vtkExtractGrid> sg3 = vtkSmartPointer<vtkExtractGrid>::New();
    sg3->SetInputConnection( transformFilter->GetOutputPort()  );
    sg3->SetSampleRate(10,10,1);
    vtkSmartPointer<vtkExtractGrid> sg4 = vtkSmartPointer<vtkExtractGrid>::New();
    sg4->SetInputConnection( transformFilter->GetOutputPort()  );
    sg4->SetSampleRate(2,2,1);

    //assign a color table
    vtkSmartPointer<vtkLookupTable> lut = View3dColorTables::getColorTable( ColorTable::RAINBOW, min, max);

    // Create mappers (visualization parameters) for each level-of-detail
    vtkSmartPointer<vtkDataSetMapper> m1 = vtkSmartPointer<vtkDataSetMapper>::New();
    m1->SetInputConnection( sg1->GetOutputPort() );
    m1->SetLookupTable(lut);
    m1->SetScalarRange(min, max);
    m1->Update();
    vtkSmartPointer<vtkDataSetMapper> m2 = vtkSmartPointer<vtkDataSetMapper>::New();
    m2->SetInputConnection( sg2->GetOutputPort() );
    m2->SetLookupTable(lut);
    m2->SetScalarRange(min, max);
    m2->Update();
    vtkSmartPointer<vtkDataSetMapper> m3 = vtkSmartPointer<vtkDataSetMapper>::New();
    m3->SetInputConnection( sg3->GetOutputPort() );
    m3->SetLookupTable(lut);
    m3->SetScalarRange(min, max);
    m3->Update();
    vtkSmartPointer<vtkDataSetMapper> m4 = vtkSmartPointer<vtkDataSetMapper>::New();
    m4->SetInputConnection( sg4->GetOutputPort() );
    m4->SetLookupTable(lut);
    m4->SetScalarRange(min, max);
    m4->Update();

    //create a LOD VTK actor, which accpets multiple mappers with possibly different
    //rendering performances
    vtkSmartPointer<vtkLODProp3D> propLOD = vtkSmartPointer<vtkLODProp3D>::New();
    propLOD->AutomaticLODSelectionOn();
    propLOD->SetLODLevel( propLOD->AddLOD(m1, 1e-12), 4.0 );
    propLOD->SetLODLevel( propLOD->AddLOD(m2, 1e-9), 3.0 );
    propLOD->SetLODLevel( propLOD->AddLOD(m3, 1e-6), 2.0 );
    propLOD->SetLODLevel( propLOD->AddLOD(m4, 1e-3), 1.0 );

    //set the rendering time criteria to switch between the several available mappers
    //since they have different rendering times due to their different details
    //propLOD->SetAllocatedRenderTime(1e-10, widget3D->getRenderer() );

    //Set up a listener to refresh events for debugging purposes (this can be removed)
    vtkSmartPointer<vtkCallbackCommand> refreshCallback =
      vtkSmartPointer<vtkCallbackCommand>::New();
    refreshCallback->SetCallback (RefreshCallback);
    refreshCallback->SetClientData(propLOD);
    widget3D->getRenderer()->GetRenderWindow()->AddObserver(vtkCommand::ModifiedEvent,refreshCallback);

    // Finally, return the actor.
    return View3DViewData(propLOD);
}

View3DViewData View3DBuilders::buildForAttributeInMapCartesianGridWithVtkStructuredGrid(CartesianGrid *cartesianGrid,
                                                                                        Attribute *attribute,
                                                                                        View3DWidget */*widget3D*/)
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
    double X0 = cartesianGrid->getX0();
    double Y0 = cartesianGrid->getY0();
    double dX = cartesianGrid->getDX();
    double dY = cartesianGrid->getDY();
    double azimuth = cartesianGrid->getRot();
    double X0frame = X0 - dX/2.0;
    double Y0frame = Y0 - dY/2.0;

    //create a VTK array to store the sample values
    vtkSmartPointer<vtkFloatArray> values = vtkSmartPointer<vtkFloatArray>::New();
    values->SetName("values");

    //create a visibility array. Cells with visibility >= 1 will be
    //visible, and < 1 will be invisible.
    vtkSmartPointer<vtkIntArray> visibility = vtkSmartPointer<vtkIntArray>::New();
    visibility->SetNumberOfComponents(1);
    visibility->SetName("Visibility");

    //read sample values and cell visibility flags
    values->Allocate( nX*nY );
    visibility->Allocate( nX*nY );
    for( int i = 0; i < nX*nY; ++i){
        // sample value
        double value = cartesianGrid->data( i, var_index - 1 );
        values->InsertNextValue( value );
        // visibility flag
        if( cartesianGrid->isNDV( value ) )
            visibility->InsertNextValue( 0 );
        else
            visibility->InsertNextValue( 1 );
    }

    //we don't need file's data anymore
    cartesianGrid->freeLoadedData();

    // set up a transform to apply the rotation about the grid origin (location of the first data point)
    vtkSmartPointer<vtkTransform> xform = vtkSmartPointer<vtkTransform>::New();
    xform->Translate( X0, Y0, 0);
    xform->RotateZ( -azimuth );
    xform->Translate( -X0, -Y0, 0);

    // Create a grid (corner-point, explicit geometry)
    //  As GSLib grids are cell-centered, then we must add an extra point in each direction
    vtkSmartPointer<vtkStructuredGrid> structuredGrid =
            vtkSmartPointer<vtkStructuredGrid>::New();
    vtkSmartPointer<vtkPoints> points =
            vtkSmartPointer<vtkPoints>::New();
    for(int k = 0; k <= 1; ++k)
        for(int j = 0; j <= nY; ++j)
            for(int i = 0; i <= nX; ++i)
                points->InsertNextPoint( X0frame + i * dX,
                                         Y0frame + j * dY,
                                         0 + k * 1 );
    structuredGrid->SetDimensions( nX+1, nY+1, 1 );
    structuredGrid->SetPoints(points);
    //assign the grid values to the grid cells
    structuredGrid->GetCellData()->SetScalars( values );
    structuredGrid->GetCellData()->AddArray( visibility );

    //apply the transform (rotation) to the grid
    vtkSmartPointer<vtkTransformFilter> transformFilter =
            vtkSmartPointer<vtkTransformFilter>::New();
    transformFilter->SetInputData( structuredGrid );
    transformFilter->SetTransform(xform);
    transformFilter->Update();

    //try a sampling rate to keep the number of elements below the threshold
    int srate = 1;
    int maxcells = Application::instance()->getMaxGridCellCountFor3DVisualizationSetting();
    for( ; (nX*nY) / (srate*srate) >  maxcells; ++srate);

    //warn user if sampling rate is less than maximum detail
    if( srate > 1){
        QString message("Grid with too many cells (");
        message += QString::number(nX*nY) +
                " > " + QString::number(maxcells) + " ).  Sampling rate set to " +
                QString::number(srate) + " (1 = max. detail)";
        QMessageBox::warning( nullptr, "Warning", message);
        Application::instance()->logWarn("View3DBuilders::buildForAttributeInMapCartesianGridWithVtkStructuredGrid: " + message);
    }

    //apply grid downscaling (if necessary)
    vtkSmartPointer<vtkExtractGrid> sg = vtkSmartPointer<vtkExtractGrid>::New();
    sg->SetInputConnection( transformFilter->GetOutputPort()  );
    sg->SetSampleRate(srate, srate, srate);
    sg->Update();

    // threshold to make unvalued cells invisible
    vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
    threshold->SetInputData(sg->GetOutput());
    threshold->ThresholdByUpper(1); // Criterion is cells whose scalars are greater or equal to threshold.
    threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Visibility");
    threshold->Update();

    //assign a color table
    vtkSmartPointer<vtkLookupTable> lut = View3dColorTables::getColorTable( ColorTable::RAINBOW, min, max);

    // Create mappers (visualization parameters) for each level-of-detail
    vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();
    mapper->SetInputConnection( threshold->GetOutputPort() );
    mapper->SetLookupTable(lut);
    mapper->SetScalarRange(min, max);
    mapper->Update();

    //create a VTK actor
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper( mapper );

    // Finally, return the actor along with other visual objects
    // so the user can make adjustments to rendering.
    return View3DViewData(actor, sg, mapper, threshold);
}

View3DViewData View3DBuilders::buildFor3DCartesianGrid(CartesianGrid *cartesianGrid, View3DWidget */*widget3D*/)
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

View3DViewData View3DBuilders::buildForAttribute3DCartesianGridWithIJKClipping(CartesianGrid *cartesianGrid,
                                                                               Attribute *attribute,
                                                                               View3DWidget */*widget3D*/)
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
    vtkSmartPointer<vtkFloatArray> values = vtkSmartPointer<vtkFloatArray>::New();
    values->SetName("values");

    //create a visibility array. Cells with visibility >= 1 will be
    //visible, and < 1 will be invisible.
    vtkSmartPointer<vtkIntArray> visibility = vtkSmartPointer<vtkIntArray>::New();
    visibility->SetNumberOfComponents(1);
    visibility->SetName("Visibility");

    //try a sampling rate to keep the number of elements below the threshold
    int srate = 1;
    int maxcells = Application::instance()->getMaxGridCellCountFor3DVisualizationSetting();
    for( ; (nX*nY*nZ) / (srate*srate*srate) >  maxcells; ++srate);

    //warn user if sampling rate is less than maximum detail
    if( srate > 1){
        QString message("Grid with too many cells (");
        message += QString::number(nX*nY*nZ) +
                " > " + QString::number(maxcells) + " ).  Sampling rate set to " +
                QString::number(srate) + " (1 = max. detail)";
        QMessageBox::warning( nullptr, "Warning", message);
        Application::instance()->logWarn("View3DBuilders::buildForAttribute3DCartesianGridWithIJKClipping: " + message);
    }

    //set sub-sampled grid dimensions
    int nXsub = nX / srate;
    int nYsub = nY / srate;
    int nZsub = nZ / srate;

    //read sample values
    values->Allocate( nXsub*nYsub*nZsub );
    visibility->Allocate( nXsub*nYsub*nZsub );
    for( int k = 0; k < nZsub; ++k){
        for( int j = 0; j < nYsub; ++j){
            for( int i = 0; i < nXsub; ++i){
                // sample value
                double value = cartesianGrid->dataIJK( var_index - 1,
                                                       std::min(i*srate, nX-1),
                                                       std::min(j*srate, nY-1),
                                                       std::min(k*srate, nZ-1) );
                values->InsertNextValue( value );
                // visibility flag
                if( cartesianGrid->isNDV( value ) )
                    visibility->InsertNextValue( 0 );
                else
                    visibility->InsertNextValue( 1 );
            }
        }
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
    for(int k = 0; k <= nZsub; ++k)
        for(int j = 0; j <= nYsub; ++j)
            for(int i = 0; i <= nXsub; ++i)
                points->InsertNextPoint( X0frame + i * dX * srate,
                                         Y0frame + j * dY * srate,
                                         Z0frame + k * dZ * srate );
    structuredGrid->SetDimensions( nXsub+1, nYsub+1, nZsub+1 );
    structuredGrid->SetPoints(points);

    //assign the grid values to the grid cells
    structuredGrid->GetCellData()->SetScalars( values );
    structuredGrid->GetCellData()->AddArray( visibility );

    //apply the transform (rotation) to the grid
    vtkSmartPointer<vtkTransformFilter> transformFilter =
            vtkSmartPointer<vtkTransformFilter>::New();
    transformFilter->SetInputData( structuredGrid );
    transformFilter->SetTransform(xform);
    transformFilter->Update();

    //apply a grid sub-sampler/re-sampler to handle clipping
    vtkSmartPointer<vtkExtractGrid> subGrid =
            vtkSmartPointer<vtkExtractGrid>::New();
    subGrid->SetInputConnection( transformFilter->GetOutputPort()  );
    subGrid->SetVOI( 0, nXsub, 0, nYsub, 0, nZsub );
    subGrid->SetSampleRate(srate, srate, srate);
    subGrid->Update();

    // threshold to make unvalued cells invisible
    vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
    threshold->SetInputData(subGrid->GetOutput());
    threshold->ThresholdByUpper(1); // Criterion is cells whose scalars are greater or equal to threshold.
    threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Visibility");
    threshold->Update();

    //assign a color table
    vtkSmartPointer<vtkLookupTable> lut = View3dColorTables::getColorTable( ColorTable::RAINBOW, min, max);

    // Create mapper (visualization parameters)
    vtkSmartPointer<vtkDataSetMapper> mapper =
            vtkSmartPointer<vtkDataSetMapper>::New();
    mapper->SetInputConnection( threshold->GetOutputPort() );
    mapper->SetLookupTable(lut);
    mapper->SetScalarRange(min, max);
    mapper->Update();

    // Finally, pass everything to the actor and return it.
    vtkSmartPointer<vtkActor> actor =
            vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    //actor->GetProperty()->EdgeVisibilityOn();
    return View3DViewData(actor, subGrid, mapper, threshold, srate);
}

View3DViewData View3DBuilders::buildForStratGrid(ProjectComponent */*toBeSpecified*/, View3DWidget */*widget3D*/)
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
