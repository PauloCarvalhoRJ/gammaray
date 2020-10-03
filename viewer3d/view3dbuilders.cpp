#include "view3dbuilders.h"
#include "domain/application.h"
#include "domain/projectcomponent.h"
#include "domain/pointset.h"
#include "domain/attribute.h"
#include "domain/cartesiangrid.h"
#include "domain/geogrid.h"
#include "domain/segmentset.h"
#include "domain/section.h"
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
#include <vtkHexahedron.h>
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
#include <vtkUnstructuredGrid.h>
#include <vtkLineSource.h>
#include <vtkTubeFilter.h>
#include <vtkLine.h>
#include <vtkQuad.h>
#include <vtkBillboardTextActor3D.h>
#include <vtkTextProperty.h>

#include <QMessageBox>
#include <QPushButton>


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
                                      + "\" not found.  Maybe it needs to implement "
                                        "ProjectComponent::build3DViewObjects().");
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

    //create an actor for the label
    double cX, cY, cZ;
    vtkSmartPointer<vtkBillboardTextActor3D> textActor;
    if( pointSet->getCenter( cX, cY, cZ ) ){
        textActor = vtkSmartPointer<vtkBillboardTextActor3D>::New();
        textActor->SetInput ( pointSet->getName().toStdString().c_str() );
        textActor->SetPosition( cX, cY, cZ );
        //text style (calls to textActor->GetTextProperty()->... to set font size, color, etc.) is
        //controlled via user settings in the View3DWidget class.
    }

    View3DViewData v3dd( actor );
    v3dd.labelActor = textActor;
    return v3dd;
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
    } else if( fileType == "SEGMENTSET" ){
        return buildForAttributeFromSegmentSet( static_cast<SegmentSet*>(file), attribute, widget3D );
    } else if( fileType == "CARTESIANGRID" ) {
        CartesianGrid* cg = (CartesianGrid*)file;
        if( ! cg->isUVWOfAGeoGrid() ){ //cg is a stand-alone Cartesian grid
			if( cg->getNZ() < 2 ){
                QMessageBox msgBox;
                msgBox.setText("Display 2D grid as?");
                QAbstractButton* pButtonUseFlat = msgBox.addButton("Flat grid at z = 0.0", QMessageBox::YesRole);
                msgBox.addButton("Surface w/ z = variable", QMessageBox::NoRole);
                msgBox.exec();
                if ( msgBox.clickedButton() == pButtonUseFlat )
                    return buildForAttributeInMapCartesianGridWithVtkStructuredGrid( cg, attribute, widget3D );
                else
                    return buildForSurfaceCartesianGrid2D( cg, attribute, widget3D );
			} else {
				return buildForAttribute3DCartesianGridWithIJKClipping( cg, attribute, widget3D );
			}
        } else { //cg is the UVW aspect of a GeoGrid: present the option to display it either with true geometry or as UVW cube
            QMessageBox msgBox;
            msgBox.setText("Which way to display the attribute?");
            QAbstractButton* pButtonUseGeoGrid = msgBox.addButton("In XYZ GeoGrid", QMessageBox::YesRole);
            msgBox.addButton("In UVW Cartesian grid", QMessageBox::NoRole);
            msgBox.exec();
            if ( msgBox.clickedButton() == pButtonUseGeoGrid )
                return buildForAttributeGeoGrid( dynamic_cast<GeoGrid*>(cg->getParent()), attribute, widget3D );
            else
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

View3DViewData View3DBuilders::build(GeoGrid * object, View3DWidget * widget3D)
{
    return buildForGeoGridMesh( object, widget3D );
}

View3DViewData View3DBuilders::build(Section *object, View3DWidget *widget3D)
{
    return buildForSection( object, widget3D );
}

vtkSmartPointer<vtkUnstructuredGrid> View3DBuilders::makeSurfaceFrom2DGridWithZvalues(
        CartesianGrid *cartesianGrid,
        Attribute *attributeWithZValues)
{
    if( cartesianGrid->getNK() > 1){
        Application::instance()->logError("View3DBuilders::makeSurfaceFrom2DGridWithZvalues(): "
                                          "grid cannot have more than one depth slice.");
        return nullptr;
    }

    if( cartesianGrid->getNI() < 2 || cartesianGrid->getNJ() < 2 ){
        Application::instance()->logError("View3DBuilders::makeSurfaceFrom2DGridWithZvalues(): "
                                          "grid must be at least 2x2.");
        return nullptr;
    }

    //get the variable index in parent data file
    uint var_index = cartesianGrid->getFieldGEOEASIndex( attributeWithZValues->getName() );

    //create a VTK array to store the sample values
    vtkSmartPointer<vtkFloatArray> values = vtkSmartPointer<vtkFloatArray>::New();
    values->SetName("values");

    //create a visibility array. Cells with visibility >= 1 will be
    //visible, and < 1 will be invisible.
    vtkSmartPointer<vtkIntArray> visibility = vtkSmartPointer<vtkIntArray>::New();
    visibility->SetNumberOfComponents(1);
    visibility->SetName("Visibility");

    //get the max and min of the selected variable
    cartesianGrid->loadData();
    double min = cartesianGrid->min( var_index-1 );
    double max = cartesianGrid->max( var_index-1 );

    //get the grid dimension of the 2D grid
    uint nI = cartesianGrid->getNI();
    uint nJ = cartesianGrid->getNJ();

    //read sample values
    values->Allocate( nI * nJ );
    visibility->Allocate( nI * nJ );
    for( int j = 0; j < nJ; ++j){
        for( int i = 0; i < nI; ++i){
            // sample value
            double value = cartesianGrid->dataIJK( var_index - 1, i, j, 0);
            values->InsertNextValue( value );
            // visibility flag
            if( cartesianGrid->isNDV( value ) )
                visibility->InsertNextValue( (int)InvisibiltyFlag::INVISIBLE_NDV_VALUE );
            else
                visibility->InsertNextValue( (int)InvisibiltyFlag::VISIBLE );
        }
    }

    // Create a VTK container with the points (mesh vertexes)
    vtkSmartPointer< vtkPoints > quadVertexes = vtkSmartPointer< vtkPoints >::New();
    quadVertexes->SetNumberOfPoints( nI * nJ );
    for( int i = 0;  i < quadVertexes->GetNumberOfPoints(); ++i ){
        double x, y, z;
        uint ii, jj, kk;
        //convert sequential index to grid coordinates
        cartesianGrid->indexToIJK( i, ii, jj, kk );
        //get cell location in space
        cartesianGrid->getCellLocation( ii, jj, 0, x, y, z );
        //get sample value
        double sampleValue = cartesianGrid->dataIJK( var_index - 1, ii, jj, 0);
        //make vertex with sample value as z
        quadVertexes->InsertPoint(i, x, y, sampleValue);
    }

    // Create a VTK unstructured grid object (unrestricted geometry)
    vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    uint nCells = ( nI - 1 ) * ( nJ - 1 );
    unstructuredGrid->Allocate( nCells );
    vtkSmartPointer< vtkQuad > quad = vtkSmartPointer< vtkQuad >::New();
    for( uint i = 0; i < nCells; ++i ) {
        uint cellJ = i / ( nI - 1 );
        quad->GetPointIds()->SetId(0, i + cellJ );
        quad->GetPointIds()->SetId(1, i + cellJ + 1 );
        quad->GetPointIds()->SetId(2, i + cellJ + nI + 1 );
        quad->GetPointIds()->SetId(3, i + cellJ + nI );
        unstructuredGrid->InsertNextCell( quad->GetCellType(), quad->GetPointIds() );
    }
    unstructuredGrid->SetPoints(quadVertexes);

    return unstructuredGrid;
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

    //create an actor for the label
    double cX, cY, cZ;
    vtkSmartPointer<vtkBillboardTextActor3D> textActor;
    if( pointSet->getCenter( cX, cY, cZ ) ){
        textActor = vtkSmartPointer<vtkBillboardTextActor3D>::New();
        textActor->SetInput ( pointSet->getName().toStdString().c_str() );
        textActor->SetPosition( cX, cY, cZ );
        //text style (calls to textActor->GetTextProperty()->... to set font size, color, etc.) is
        //controlled via user settings in the View3DWidget class.
    }

    View3DViewData v3dd( actor );
    v3dd.labelActor = textActor;
    return v3dd;
}

View3DViewData View3DBuilders::buildForAttributeFromSegmentSet(SegmentSet *segmentSet,
                                                               Attribute *attribute,
                                                               View3DWidget *widget3D)
{
    //load data from filesystem
    segmentSet->loadData();

    //get the array indexes for the xyz coordinates
    //defining the segments
    uint x0colIdx = segmentSet->getXindex() - 1;
    uint y0colIdx = segmentSet->getYindex() - 1;
    uint z0colIdx = segmentSet->getZindex() - 1;
    uint x1colIdx = segmentSet->getXFinalIndex() - 1;
    uint y1colIdx = segmentSet->getYFinalIndex() - 1;
    uint z1colIdx = segmentSet->getZFinalIndex() - 1;

    //get the array index of the target variable in parent data file
    uint var_index = segmentSet->getFieldGEOEASIndex( attribute->getName() );

    //create a VTK array to store the sample values
    vtkSmartPointer<vtkFloatArray> values = vtkSmartPointer<vtkFloatArray>::New();
    values->SetName("values");

    //get the max and min of the selected variable (useful for continuous variables)
    double min = segmentSet->min( var_index-1 );
    double max = segmentSet->max( var_index-1 );

    //build point and segment primitives from data
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> segments = vtkSmartPointer<vtkCellArray>::New();
    for( uint i = 0; i < segmentSet->getDataLineCount(); ++i ){
        double x0 = segmentSet->data( i, x0colIdx );
        double y0 = segmentSet->data( i, y0colIdx );
        double z0 = segmentSet->data( i, z0colIdx );
        double x1 = segmentSet->data( i, x1colIdx );
        double y1 = segmentSet->data( i, y1colIdx );
        double z1 = segmentSet->data( i, z1colIdx );
        vtkIdType id0 = points->InsertNextPoint( x0, y0, z0 );
        vtkIdType id1 = points->InsertNextPoint( x1, y1, z1 );
        vtkSmartPointer<vtkLine> segment = vtkSmartPointer<vtkLine>::New();
        segment->GetPointIds()->SetId( 0, id0 );
        segment->GetPointIds()->SetId( 1, id1 );
        segments->InsertNextCell( segment );
        // take the opportunitu to load the sample values
        double value = segmentSet->data( i, var_index - 1 );
        values->InsertNextValue( value );
    }

    // build a polygonal line from the points and segments primitives
    vtkSmartPointer<vtkPolyData> poly = vtkSmartPointer<vtkPolyData>::New();
    poly->SetPoints( points );
    poly->SetLines( segments );
    poly->GetCellData()->SetScalars( values );
    poly->GetCellData()->SetActiveScalars("values");

    // build a tube around the polygonal line
    vtkSmartPointer<vtkTubeFilter> tubeFilter =
      vtkSmartPointer<vtkTubeFilter>::New();
    tubeFilter->SetInputData( poly );
    tubeFilter->SetRadius(10); //default is .5
    tubeFilter->SetNumberOfSides(50);
    tubeFilter->Update();

    //create a color table according to variable type (continuous or categorical)
    vtkSmartPointer<vtkLookupTable> lut;
    if( attribute->isCategorical() )
        lut = View3dColorTables::getCategoricalColorTable( segmentSet->getCategoryDefinition( attribute ), false );
    else
        lut = View3dColorTables::getColorTable( ColorTable::RAINBOW, min, max );

    // Create a VTK mapper and actor to enable its visualization
    vtkSmartPointer<vtkPolyDataMapper> tubeMapper =
      vtkSmartPointer<vtkPolyDataMapper>::New();
    tubeMapper->SetInputConnection(tubeFilter->GetOutputPort());
    tubeMapper->SetLookupTable(lut);
    tubeMapper->SetScalarModeToUseCellData();
    tubeMapper->SetColorModeToMapScalars();
    tubeMapper->SelectColorArray("values");
    tubeMapper->SetScalarRange(min, max);

    vtkSmartPointer<vtkActor> tubeActor =
      vtkSmartPointer<vtkActor>::New();
    tubeActor->GetProperty()->SetOpacity(1.0); //Make the tube have some transparency.
    tubeActor->SetMapper(tubeMapper);

    //create an actor for the label
    double cX, cY, cZ;
    vtkSmartPointer<vtkBillboardTextActor3D> textActor;
    if( segmentSet->getCenter( cX, cY, cZ ) ){
        textActor = vtkSmartPointer<vtkBillboardTextActor3D>::New();
        textActor->SetInput ( segmentSet->getName().toStdString().c_str() );
        textActor->SetPosition( cX, cY, cZ );
        //text style (calls to textActor->GetTextProperty()->... to set font size, color, etc.) is
        //controlled via user settings in the View3DWidget class.
    }

    View3DViewData v3dd( tubeActor );
    v3dd.tubeFilter = tubeFilter;
    v3dd.labelActor = textActor;
    return v3dd;
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
    plane->SetPoint1( X0frame + nX * dX, Y0frame, 0.0);
    plane->SetPoint2( X0frame, Y0frame + nY * dY, 0.0);
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

    // Show grid geometry as a wireframe
    actor->GetProperty()->EdgeVisibilityOn();
    actor->GetProperty()->BackfaceCullingOn();
    actor->GetProperty()->FrontfaceCullingOn();

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
            visibility->InsertNextValue( (int)InvisibiltyFlag::INVISIBLE_NDV_VALUE );
        else
            visibility->InsertNextValue( (int)InvisibiltyFlag::VISIBLE );
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

    //create a color table according to variable type (continuous or categorical)
    vtkSmartPointer<vtkLookupTable> lut;
    if( attribute->isCategorical() )
        lut = View3dColorTables::getCategoricalColorTable( cartesianGrid->getCategoryDefinition( attribute ), false );
    else
        lut = View3dColorTables::getColorTable( ColorTable::RAINBOW, min, max );

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
                //the ( d* + d*/n* ) is to account for the extra cells in each direction due
                //due to cell-centered-to-corner-point conversion
                points->InsertNextPoint( X0frame + i * ( dX + dX/nX ),
                                         Y0frame + j * ( dY + dY/nY ),
                                         Z0frame + k * ( dZ + dZ/nZ ) );
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

    // Show grid geometry as a wireframe
    actor->GetProperty()->EdgeVisibilityOn();
    actor->GetProperty()->SetRepresentationToWireframe();

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
                    visibility->InsertNextValue( (int)InvisibiltyFlag::INVISIBLE_NDV_VALUE );
                else
                    visibility->InsertNextValue( (int)InvisibiltyFlag::VISIBLE );
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
                //the ( d* + d*/n*sub ) is to account for the extra cells in each direction
                //due to the corner-point-to-cell-centered conversion
                points->InsertNextPoint( X0frame + i * ( dX + dX/nXsub ) * srate,
                                         Y0frame + j * ( dY + dY/nYsub ) * srate,
                                         Z0frame + k * ( dZ + dZ/nZsub ) * srate );
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

    //create a color table according to variable type (continuous or categorical)
    vtkSmartPointer<vtkLookupTable> lut;
    if( attribute->isCategorical() )
        lut = View3dColorTables::getCategoricalColorTable( cartesianGrid->getCategoryDefinition( attribute ), false );
    else
        lut = View3dColorTables::getColorTable( ColorTable::RAINBOW, min, max );

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

View3DViewData View3DBuilders::buildForGeoGridMesh( GeoGrid * geoGrid, View3DWidget * widget3D )
{
	Q_UNUSED( widget3D );

	// Create a VTK container with the points (mesh vertexes)
	vtkSmartPointer< vtkPoints > hexaPoints = vtkSmartPointer< vtkPoints >::New();
	hexaPoints->SetNumberOfPoints( geoGrid->getMeshNumberOfVertexes() );
	for( int i = 0;  i < hexaPoints->GetNumberOfPoints(); ++i ){
		double x, y, z;
		geoGrid->getMeshVertexLocation( i, x, y, z );
		hexaPoints->InsertPoint(i, x, y, z);
	}

	// Create a VTK unstructured grid object (allows faults, erosions, and other geologic discordances )
	vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
	uint nCells = geoGrid->getMeshNumberOfCells();
	unstructuredGrid->Allocate( nCells );
    vtkSmartPointer< vtkHexahedron > hexa = vtkSmartPointer< vtkHexahedron >::New();
    for( uint i = 0; i < nCells; ++i ) {
		uint vIds[8];
		geoGrid->getMeshCellDefinition( i, vIds );
		hexa->GetPointIds()->SetId(0, vIds[0]);
		hexa->GetPointIds()->SetId(1, vIds[1]);
		hexa->GetPointIds()->SetId(2, vIds[2]);
		hexa->GetPointIds()->SetId(3, vIds[3]);
		hexa->GetPointIds()->SetId(4, vIds[4]);
		hexa->GetPointIds()->SetId(5, vIds[5]);
		hexa->GetPointIds()->SetId(6, vIds[6]);
		hexa->GetPointIds()->SetId(7, vIds[7]);
		unstructuredGrid->InsertNextCell(hexa->GetCellType(), hexa->GetPointIds());
	}
	unstructuredGrid->SetPoints(hexaPoints);

	// Create a mapper and actor
    vtkSmartPointer<vtkDataSetMapper> mapper =
            vtkSmartPointer<vtkDataSetMapper>::New();
	mapper->SetInputData(unstructuredGrid);

    vtkSmartPointer<vtkActor> actor =
            vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

	return View3DViewData( actor );
}

View3DViewData View3DBuilders::buildForAttributeGeoGrid( GeoGrid * geoGrid, Attribute * attribute, View3DWidget* widget3D )
{
	Q_UNUSED( widget3D );

	//get the variable index in parent data file
	uint var_index = geoGrid->getFieldGEOEASIndex( attribute->getName() );

	//create a VTK array to store the sample values
	vtkSmartPointer<vtkFloatArray> values = vtkSmartPointer<vtkFloatArray>::New();
	values->SetName("values");

	//create a visibility array. Cells with visibility >= 1 will be
	//visible, and < 1 will be invisible.
	vtkSmartPointer<vtkIntArray> visibility = vtkSmartPointer<vtkIntArray>::New();
	visibility->SetNumberOfComponents(1);
	visibility->SetName("Visibility");

	//get the max and min of the selected variable
	geoGrid->loadData();
    double min = geoGrid->min( var_index-1 );
    double max = geoGrid->max( var_index-1 );

	//get the grid dimension of the GeoGrid
	uint nI = geoGrid->getNI();
	uint nJ = geoGrid->getNJ();
	uint nK = geoGrid->getNK();

	//read sample values
	values->Allocate( nI * nJ * nK );
	visibility->Allocate( nI * nJ * nK );
	for( int k = 0; k < nK; ++k){
		for( int j = 0; j < nJ; ++j){
			for( int i = 0; i < nI; ++i){
				// sample value
				double value = geoGrid->dataIJK( var_index - 1, i, j, k);
				values->InsertNextValue( value );
				// visibility flag
				if( geoGrid->isNDV( value ) )
                    visibility->InsertNextValue( (int)InvisibiltyFlag::INVISIBLE_NDV_VALUE );
				else
                    visibility->InsertNextValue( (int)InvisibiltyFlag::VISIBLE );
			}
		}
	}

	// Create a VTK container with the points (mesh vertexes)
	vtkSmartPointer< vtkPoints > hexaPoints = vtkSmartPointer< vtkPoints >::New();
	hexaPoints->SetNumberOfPoints( geoGrid->getMeshNumberOfVertexes() );
	for( int i = 0;  i < hexaPoints->GetNumberOfPoints(); ++i ){
		double x, y, z;
		geoGrid->getMeshVertexLocation( i, x, y, z );
		hexaPoints->InsertPoint(i, x, y, z);
	}

	// Create a VTK unstructured grid object (allows faults, erosions, and other geologic discordances )
	vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
	uint nCells = geoGrid->getMeshNumberOfCells();
	unstructuredGrid->Allocate( nCells );
    vtkSmartPointer< vtkHexahedron > hexa = vtkSmartPointer< vtkHexahedron >::New();
    for( uint i = 0; i < nCells; ++i ) {
        uint vIds[8];
        geoGrid->getMeshCellDefinition( i, vIds );
        hexa->GetPointIds()->SetId(0, vIds[0]);
        hexa->GetPointIds()->SetId(1, vIds[1]);
        hexa->GetPointIds()->SetId(2, vIds[2]);
        hexa->GetPointIds()->SetId(3, vIds[3]);
        hexa->GetPointIds()->SetId(4, vIds[4]);
        hexa->GetPointIds()->SetId(5, vIds[5]);
        hexa->GetPointIds()->SetId(6, vIds[6]);
        hexa->GetPointIds()->SetId(7, vIds[7]);
        unstructuredGrid->InsertNextCell(hexa->GetCellType(), hexa->GetPointIds());
	}
    unstructuredGrid->SetPoints(hexaPoints);

	//assign the grid values to the grid cells
	unstructuredGrid->GetCellData()->SetScalars( values );
	unstructuredGrid->GetCellData()->AddArray( visibility );

    // threshold to make unvalued cells invisible
	vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
	threshold->SetInputData( unstructuredGrid );
	threshold->ThresholdByUpper(1); // Criterion is cells whose scalars are greater or equal to threshold.
	threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Visibility");
	threshold->Update();

    //create a color table according to variable type (continuous or categorical)
    vtkSmartPointer<vtkLookupTable> lut;
    if( attribute->isCategorical() )
        lut = View3dColorTables::getCategoricalColorTable( geoGrid->getCategoryDefinition( attribute ), false );
    else
        lut = View3dColorTables::getColorTable( ColorTable::RAINBOW, min, max );

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

    return View3DViewData( actor, mapper, threshold );
}

View3DViewData View3DBuilders::buildForSurfaceCartesianGrid2D(CartesianGrid *cartesianGrid,
                                                              Attribute *attribute,
                                                              View3DWidget *widget3D)
{
    Q_UNUSED( widget3D )

    // Make surface object from the input data.
    vtkSmartPointer< vtkUnstructuredGrid > unstructuredGrid =
            makeSurfaceFrom2DGridWithZvalues( cartesianGrid, attribute );

    if( ! unstructuredGrid ){
        Application::instance()->logError("View3DBuilders::buildForSurfaceCartesianGrid2D(): "
                                          "null surface.  Check output pane for error messages.");
        return View3DViewData();
    }

    // Create mapper (visualization parameters)
    vtkSmartPointer<vtkDataSetMapper> mapper =
            vtkSmartPointer<vtkDataSetMapper>::New();
    mapper->SetInputData( unstructuredGrid );
    mapper->Update();

    // Finally, pass everything to the actor and return it.
    vtkSmartPointer<vtkActor> actor =
            vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    //actor->GetProperty()->EdgeVisibilityOn();

    return View3DViewData( actor );
}

View3DViewData View3DBuilders::buildForSection(Section *section, View3DWidget *widget3D)
{
    Q_UNUSED( widget3D )

    vtkSmartPointer<vtkStructuredGrid> structuredGrid = vtkSmartPointer<vtkStructuredGrid>::New();
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

//    double length = 10;
//    double rangeAngle = 180;
//    double option = 2;

//    int dAngle = 10;
//    int dLength = 10;

//    double angleIncrease = rangeAngle / dAngle; //120.0 / gridSize;
//    double lengthIncrease = length / dLength; //120.0 / gridSize;

//    for (unsigned int r = 0; r < length; r = r + lengthIncrease)
//    {
//        for (double theta = 0; theta <= 180; theta = theta + angleIncrease)
//        {
//            r = sqrt( r*r );
//            double xx = cos(theta*(3.14159 / 180.0))*(r + option);
//            double yy = sin(theta*(3.14159 / 180.0))*(r + option);
//            points->InsertNextPoint(xx, yy, 0); // Make most of the points the same height
//        }
//    }

//    // Specify the dimensions of the grid
//    structuredGrid->SetDimensions(dAngle+1, dLength, 1);
//    structuredGrid->SetPoints(points);


    // Get the Section's component data files.
    PointSet* sectionPathPS = section->getPointSet();
    CartesianGrid* sectionDataCG = section->getCartesianGrid();

    // Sanity checks.
    if( ! sectionPathPS ){
        Application::instance()->logError("View3DBuilders::buildForSection(): Section without a point set "
                                          "defining its geographic path.", true);
        return View3DViewData();
    }
    if( ! sectionDataCG ){
        Application::instance()->logError("View3DBuilders::buildForSection(): Section without a Cartesian grid "
                                          "containing its data.", true);
        return View3DViewData();
    }

    // Load the data files.
    sectionPathPS->loadData();
    sectionDataCG->loadData();

    // Get geometry data.
    int nCols = sectionDataCG->getNI();
    int nRows = sectionDataCG->getNK(); //number of rows of a section is the number of layers of its data grid.
    int nSegments = sectionPathPS->getDataLineCount()-1; //if the PS has two entries, it defines one section segment.

    // Sanity check.
    if( nSegments < 1 ){
        Application::instance()->logError("View3DBuilders::buildForSection(): Section must have at least one segment.", true);
        return View3DViewData();
    }

    // For each horizon (from top of the "fence" to the bottom).
    for( int iHorizon = 0; iHorizon < nRows+1; ++iHorizon ){

        // For each section segment.
        for( int iSegment = 0;  iSegment < nSegments; ++iSegment){
            //See documentation of the Section class for point set convention regarding variable order.
            double segment_Xi    = sectionPathPS->data( iSegment,   0 ); //X is 1st variable per the section format
            double segment_Yi    = sectionPathPS->data( iSegment,   1 ); //Y is 2nd variable per the section format
            double segment_topZi = sectionPathPS->data( iSegment,   2 ); //Z1 is 3rd variable per the section format
            double segment_botZi = sectionPathPS->data( iSegment,   3 ); //Z2 is 4th variable per the section format
            double segment_Xf    = sectionPathPS->data( iSegment+1, 0 );
            double segment_Yf    = sectionPathPS->data( iSegment+1, 1 );
            double segment_topZf = sectionPathPS->data( iSegment+1, 2 );
            double segment_botZf = sectionPathPS->data( iSegment+1, 3 );
            int nSegmentCols     = sectionPathPS->data( iSegment,   4 ); //data grid column is 5th variable per the section format


            //For each column divider in the segment
            {

            } // For each column divider in the segment
        } // For each section segment
    } // For each horizon




    // Specify the dimensions of the grid.
    structuredGrid->SetDimensions(nCols+1, nRows+1, 1);
    structuredGrid->SetPoints(points);

    // Create a mapper for the vtkStructuredGrid.
    vtkSmartPointer<vtkDataSetMapper> gridMapper = vtkSmartPointer<vtkDataSetMapper>::New();
    gridMapper->SetInputData(structuredGrid);

    // Create an actor for the scene in the main 3D view widget.
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(gridMapper);
    actor->GetProperty()->EdgeVisibilityOn();
    actor->GetProperty()->SetRepresentationToWireframe();
    actor->GetProperty()->SetEdgeColor(0, 0, 1);

    return View3DViewData( actor );
}
