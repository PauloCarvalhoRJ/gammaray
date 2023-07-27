//----------Since we're not building with CMake, we need to init the VTK
// modules------------------
//--------------linking with the VTK libraries is often not
// enough--------------------------------
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2); // Assuming VTK was built with OpenGL2 rendering backend
//------------------------------------------------------------------------------------------------

#include "view3dbuilders.h"
#include "domain/application.h"
#include "domain/projectcomponent.h"
#include "domain/pointset.h"
#include "domain/attribute.h"
#include "domain/cartesiangrid.h"
#include "domain/geogrid.h"
#include "domain/segmentset.h"
#include "domain/section.h"
#include "domain/categorydefinition.h"
#include "dialogs/choosevariabledialog.h"
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
#include <vtkImageData.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolumeProperty.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkContourFilter.h>

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

    View3DViewData v3dd( pointSet, pointCloud, actor );
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
        if( cg->isUVWOfAGeoGrid() ){ //cg is the UVW aspect of a GeoGrid: present the option to display it
                                     // either with true geometry or as UVW cube
            QMessageBox msgBox;
            msgBox.setText("Which way to display the attribute?");
            QAbstractButton* pButtonUseGeoGrid = msgBox.addButton("In XYZ GeoGrid", QMessageBox::YesRole);
            msgBox.addButton("In UVW Cartesian grid", QMessageBox::NoRole);
            msgBox.exec();
            if ( msgBox.clickedButton() == pButtonUseGeoGrid )
                return buildForAttributeGeoGrid( dynamic_cast<GeoGrid*>(cg->getParent()), attribute, widget3D );
            else
                return buildForAttribute3DCartesianGridUserChoice( cg, attribute, widget3D );
        } else if( cg->isDataStoreOfaGeologicSection() ) { //cg is the data storage of a Section: present
                                                           // the option to display it either with true geometry
                                                           //or as plain cube
            QMessageBox msgBox;
            msgBox.setText("Which way to display the attribute?");
            QAbstractButton* pButtonUseSection = msgBox.addButton("In geologic section", QMessageBox::YesRole);
            msgBox.addButton("As Cartesian grid", QMessageBox::NoRole);
            msgBox.exec();
            if ( msgBox.clickedButton() == pButtonUseSection )
                return buildForAttributeSection( dynamic_cast<Section*>(cg->getParent()), attribute, widget3D );
            else
                return buildForAttribute3DCartesianGridUserChoice( cg, attribute, widget3D );
        } else { //cg is a stand-alone Cartesian grid
            if( cg->getNZ() < 2 ){
                QMessageBox msgBox;
                msgBox.setText("Display 2D grid as a surface:");
                QAbstractButton* pButtonUseFlat = msgBox.addButton("z = 0.0", QMessageBox::YesRole);
                QAbstractButton* pButtonUseZOwn = msgBox.addButton("z = " +
                                                                   attribute->getName(), QMessageBox::NoRole);
                QAbstractButton* pButtonUseZPainted = msgBox.addButton("painted w/ another variable", QMessageBox::NoRole);
                QAbstractButton* pButtonUseContours = msgBox.addButton("as contour lines", QMessageBox::NoRole);
                msgBox.exec();
                if ( msgBox.clickedButton() == pButtonUseFlat )
                    return buildForAttributeInMapCartesianGridWithVtkStructuredGrid( cg, attribute, widget3D );
                else if( msgBox.clickedButton() == pButtonUseZOwn )
                    return buildForSurfaceCartesianGrid2D( cg, attribute, widget3D );
                else if( msgBox.clickedButton() == pButtonUseZPainted ){
                    //The user must choose another variable of the same dataset to paint the surface with.
                    ChooseVariableDialog cvd( cg, "Select variable.", "Paint surface with:", false );
                    cvd.exec();
                    Attribute* attributePaintWith = cg->getAttributeFromGEOEASIndex( cvd.getSelectedVariableIndex()+1 );
                    return buildForSurfaceCartesianGrid2Dpainted( cg, attribute, attributePaintWith, widget3D );
                }else if( msgBox.clickedButton() == pButtonUseContours ){
                    return buildForAttribute2DContourLines( cg, attribute, widget3D );
                }
            } else {
                return buildForAttribute3DCartesianGridUserChoice( cg, attribute, widget3D );
            }
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
        Attribute *attributeWithZValues,
        Attribute *attributeToPaintWith)
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
    uint var_index_zvals = cartesianGrid->getFieldGEOEASIndex( attributeWithZValues->getName() );
    uint var_index_paint = 0;
    if( attributeToPaintWith )
        var_index_paint = cartesianGrid->getFieldGEOEASIndex( attributeToPaintWith->getName() );

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
    double minPaint = -std::numeric_limits<double>::max();
    double maxPaint =  std::numeric_limits<double>::max();
    if( var_index_paint ) {
        minPaint = cartesianGrid->min( var_index_paint-1 );
        maxPaint = cartesianGrid->max( var_index_paint-1 );
    }

    //get the grid dimension of the 2D grid
    uint nI = cartesianGrid->getNI();
    uint nJ = cartesianGrid->getNJ();

    //read sample values of the paint variable
    if( var_index_paint ) { //if there is an attribute to paint the surface with.
        values->Allocate( nI * nJ );
        visibility->Allocate( nI * nJ );
        for( int j = 0; j < nJ; ++j){
            for( int i = 0; i < nI; ++i){
                // sample value
                double value = cartesianGrid->dataIJK( var_index_paint - 1, i, j, 0);
                values->InsertNextValue( value );
                // visibility flag
                if( cartesianGrid->isNDV( value ) )
                    visibility->InsertNextValue( (int)InvisibiltyFlag::INVISIBLE_NDV_VALUE );
                else
                    visibility->InsertNextValue( (int)InvisibiltyFlag::VISIBLE );
            }
        }
    } else { //hide vertexes whose Z values are invalid (no-data values)
        visibility->Allocate( nI * nJ );
        for( int j = 0; j < nJ; ++j){
            for( int i = 0; i < nI; ++i){
                // Z value
                double z_value = cartesianGrid->dataIJK( var_index_zvals - 1, i, j, 0);
                // visibility flag
                if( cartesianGrid->isNDV( z_value ) )
                    visibility->InsertNextValue( (int)InvisibiltyFlag::INVISIBLE_NDV_VALUE );
                else
                    visibility->InsertNextValue( (int)InvisibiltyFlag::VISIBLE );
            }
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
        double sampleValue = cartesianGrid->dataIJK( var_index_zvals - 1, ii, jj, 0);
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

    if( var_index_paint ) { //if there is an attribute to paint the surface with.
        //assign the grid values to the grid vertexes
        unstructuredGrid->GetPointData()->SetScalars( values     );
        unstructuredGrid->GetPointData()->AddArray  ( visibility );
    } else { // hide vertexes whose Z values are invalid (no-data-value)
        unstructuredGrid->GetPointData()->AddArray  ( visibility );
    }

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

    //create a color table according to variable type (continuous or categorical)
    vtkSmartPointer<vtkLookupTable> lut;
    if( attribute->isCategorical() )
        lut = View3dColorTables::getCategoricalColorTable( pointSet->getCategoryDefinition( attribute ), false );
    else
        lut = View3dColorTables::getColorTable( ColorTable::RAINBOW, min, max );

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

    View3DViewData v3dd( pointSet, pointCloud, actor );
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

    //if the attribute is categorical, retrieve the object with the categorical definitions.
    CategoryDefinition* cd = nullptr;
    if( attribute->isCategorical() ) {
        cd = segmentSet->getCategoryDefinition( attribute );
        cd->loadQuintuplets();
    }

    //create a vector for possible caption text actors
    std::vector< vtkSmartPointer<vtkBillboardTextActor3D> > captionActors;

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
        // take the opportunity to load the sample values
        double value = segmentSet->data( i, var_index - 1 );
        values->InsertNextValue( value );
        // if the attribute is categorical, add text actors with the category name
        if( attribute->isCategorical() ){
            //compute the segment's mid point.
            double cX, cY, cZ;
            segmentSet->getSegmentCenter( i, cX, cY, cZ );
            //get the category name text
            QString caption = cd->getCategoryNameByCode( value );
            //create the text actor itself and ads it to the collection of caption texts
            vtkSmartPointer<vtkBillboardTextActor3D> captionActor = vtkSmartPointer<vtkBillboardTextActor3D>::New();
            captionActor->SetInput ( caption.toStdString().c_str() );
            captionActor->SetPosition( cX, cY, cZ );
            //text style (calls to textActor->GetTextProperty()->... to set font size, color, etc.) is
            //controlled via user settings in the View3DWidget class.
            captionActors.push_back( captionActor );
        }
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
        lut = View3dColorTables::getCategoricalColorTable( cd, false );
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

    View3DViewData v3dd( segmentSet, tubeFilter->GetOutput(), tubeActor );
    v3dd.tubeFilter = tubeFilter;
    v3dd.labelActor = textActor;
    v3dd.captionActors = captionActors;
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

    return View3DViewData(cartesianGrid, transformFilter->GetOutput(), actor);
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
    threshold->SetUpperThreshold(1); // Criterion is cells whose scalars are greater or equal to threshold.
    threshold->SetThresholdFunction( vtkThreshold::THRESHOLD_UPPER );
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
    return View3DViewData(cartesianGrid, threshold->GetOutput(), actor, sg, mapper, threshold);
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

    return View3DViewData(cartesianGrid, transformFilter->GetOutput(), actor);
}

View3DViewData View3DBuilders::buildForAttribute3DCartesianGridUserChoice(CartesianGrid *cartesianGrid,
                                                                          Attribute *attribute,
                                                                          View3DWidget *widget3D)
{
    QMessageBox msgBox;
    msgBox.setText("How to display the 3D Cartesian grid?\n"
                   "-The classic rendering has a higher memory footprint but enables current features.\n"
                   "-The volumetric option is recommended for large data sets but is still experimental.");
    QAbstractButton* pButtonUseClassic = msgBox.addButton("classic", QMessageBox::YesRole);
    /* QAbstractButton* pButtonUseVolumetric = */ msgBox.addButton("volumetric", QMessageBox::NoRole);
    msgBox.exec();
    if ( msgBox.clickedButton() == pButtonUseClassic )
        return buildForAttribute3DCartesianGridWithIJKClipping( cartesianGrid, attribute, widget3D );
    else
        return buildForAttribute3DCGridIJKClippingVolumetric( cartesianGrid, attribute, widget3D );

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
    threshold->SetUpperThreshold(1); // Criterion is cells whose scalars are greater or equal to threshold.
    threshold->SetThresholdFunction( vtkThreshold::THRESHOLD_UPPER );
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
    return View3DViewData(cartesianGrid, threshold->GetOutput(), actor, subGrid, mapper, threshold, srate);
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

    return View3DViewData( geoGrid, unstructuredGrid, actor );
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
    threshold->SetUpperThreshold(1); // Criterion is cells whose scalars are greater or equal to threshold.
    threshold->SetThresholdFunction( vtkThreshold::THRESHOLD_UPPER );
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

    return View3DViewData( geoGrid, threshold->GetOutput(), actor, mapper, threshold );
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

    // threshold to make vertexes with unvalued Z's invisible
    vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
    threshold->SetInputData( unstructuredGrid );
    threshold->SetUpperThreshold(1); // Criterion is cells whose scalars are greater or equal to threshold.
    threshold->SetThresholdFunction( vtkThreshold::THRESHOLD_UPPER );
    threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Visibility");
    threshold->Update();

    // Create mapper (visualization parameters)
    vtkSmartPointer<vtkDataSetMapper> mapper =
            vtkSmartPointer<vtkDataSetMapper>::New();
    mapper->SetInputConnection( threshold->GetOutputPort() );
    mapper->Update();

    // Finally, pass everything to the actor and return it.
    vtkSmartPointer<vtkActor> actor =
            vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    //actor->GetProperty()->EdgeVisibilityOn();

    return View3DViewData( cartesianGrid, threshold->GetOutput(), actor );
}

View3DViewData View3DBuilders::buildForSurfaceCartesianGrid2Dpainted(CartesianGrid *cartesianGrid,
                                                                     Attribute *attributeZ,
                                                                     Attribute *attributePaintWith,
                                                                     View3DWidget *widget3D)
{
    // Make surface object from the input data.
    vtkSmartPointer< vtkUnstructuredGrid > unstructuredGrid =
            makeSurfaceFrom2DGridWithZvalues( cartesianGrid, attributeZ, attributePaintWith );

    if( ! unstructuredGrid ){
        Application::instance()->logError("View3DBuilders::buildForSurfaceCartesianGrid2Dpainted(): "
                                          "null surface.  Check output pane for error messages.");
        return View3DViewData();
    }

    //get the max and min of the selected variable to paint with.
    cartesianGrid->loadData();
    uint var_index_paint = cartesianGrid->getFieldGEOEASIndex( attributePaintWith->getName() );
    double minPaint = cartesianGrid->min( var_index_paint-1 );
    double maxPaint = cartesianGrid->max( var_index_paint-1 );

    //create a color table according to the type (continuous or categorical) of the variable
    //to paint with.
    vtkSmartPointer<vtkLookupTable> lut;
    if( attributePaintWith->isCategorical() )
        lut = View3dColorTables::getCategoricalColorTable(
                    cartesianGrid->getCategoryDefinition( attributePaintWith ), false, 0.0 );
    else
        lut = View3dColorTables::getColorTable( ColorTable::RAINBOW, minPaint, maxPaint );

    // threshold to make vertexes with unvalued Z's invisible
    vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
    threshold->SetInputData( unstructuredGrid );
    threshold->SetUpperThreshold(1); // Criterion is cells whose scalars are greater or equal to threshold.
    threshold->SetThresholdFunction( vtkThreshold::THRESHOLD_UPPER );
    threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Visibility");
    threshold->Update();

    // Create mapper (visualization parameters)
    vtkSmartPointer<vtkDataSetMapper> mapper =
            vtkSmartPointer<vtkDataSetMapper>::New();
    mapper->SetInputConnection( threshold->GetOutputPort() );
    mapper->SetLookupTable(lut);
    mapper->SetScalarRange(minPaint, maxPaint);
    mapper->ScalarVisibilityOn();
    mapper->Update();

    // Finally, pass everything to the actor and return it.
    vtkSmartPointer<vtkActor> actor =
            vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    return View3DViewData( cartesianGrid, threshold->GetOutput(), actor, mapper );
}

View3DViewData View3DBuilders::buildForSection(Section *section, View3DWidget *widget3D)
{
    Q_UNUSED( widget3D )

    //Make VTK object representing the geometry of the geologic section (looks like a fence).
    vtkSmartPointer<vtkStructuredGrid> structuredGrid = makeSurfaceFromSection( section );

    //sanity check.
    if( ! structuredGrid ){
        Application::instance()->logError("View3DBuilders::buildForSection(): visual representation object not created. "
                                          "Check previous messages for reasons.", true);
        return View3DViewData();
    }

    // Create a mapper for the vtkStructuredGrid.
    vtkSmartPointer<vtkDataSetMapper> gridMapper = vtkSmartPointer<vtkDataSetMapper>::New();
    gridMapper->SetInputData(structuredGrid);

    // Create an actor for the scene in the main 3D view widget.
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(gridMapper);
    actor->GetProperty()->EdgeVisibilityOn();
    actor->GetProperty()->SetRepresentationToWireframe();
    actor->GetProperty()->SetEdgeColor(0, 0, 1);

    /** Section is not actually a data file.  It is a composition of two data files (see docs). */
    return View3DViewData( section->getCartesianGrid(), structuredGrid, actor );
}

View3DViewData View3DBuilders::buildForAttributeSection(Section *section, Attribute *attribute, View3DWidget *widget3D)
{
    Q_UNUSED( widget3D )

    //Make VTK object representing the geometry of the geologic section (looks like a fence).
    vtkSmartPointer<vtkStructuredGrid> structuredGrid = makeSurfaceFromSection( section );

    //sanity check.
    if( ! structuredGrid ){
        Application::instance()->logError("View3DBuilders::buildForAttributeSection(): visual representation "
                                          "object not created. "
                                          "Check previous messages for reasons.", true);
        return View3DViewData();
    }

    //assumes the parent object of the Attribute is a Cartesian grid.
    CartesianGrid* cartesianGrid = dynamic_cast< CartesianGrid* >( attribute->getParent() );

    //sanity check.
    if( ! cartesianGrid ){
        Application::instance()->logError("View3DBuilders::buildForAttributeSection(): attribute's parent object"
                                          " is not a Cartesian grid.", true);
        return View3DViewData();
    }

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

    //create a VTK array to store the sample values
    vtkSmartPointer<vtkFloatArray> values = vtkSmartPointer<vtkFloatArray>::New();
    values->SetName("values");

    //create a visibility array. Cells with visibility >= 1 will be
    //visible, and < 1 will be invisible.
    vtkSmartPointer<vtkIntArray> visibility = vtkSmartPointer<vtkIntArray>::New();
    visibility->SetNumberOfComponents(1);
    visibility->SetName("Visibility");

    //read sample values
    values->Allocate( nX * nY * nZ );
    visibility->Allocate( nX * nY * nZ );
    for( int k = 0; k < nZ; ++k){
        for( int j = 0; j < nY; ++j){ //well, nY is always 1 for a geologic section.
            for( int i = 0; i < nX; ++i){
                // sample value
                double value = cartesianGrid->dataIJK( var_index - 1,
                                                       i,
                                                       j,
                                                       k );
                values->InsertNextValue( value );
                // visibility flag
                if( cartesianGrid->isNDV( value ) )
                    visibility->InsertNextValue( (int)InvisibiltyFlag::INVISIBLE_NDV_VALUE );
                else
                    visibility->InsertNextValue( (int)InvisibiltyFlag::VISIBLE );
            }
        }
    }

    //assign the grid values to the grid cells
    structuredGrid->GetCellData()->SetScalars( values );
    structuredGrid->GetCellData()->AddArray( visibility );

    // threshold to make unvalued cells invisible
    vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
    threshold->SetInputData( structuredGrid );
    threshold->SetUpperThreshold(1); // Criterion is cells whose scalars are greater or equal to threshold.
    threshold->SetThresholdFunction( vtkThreshold::THRESHOLD_UPPER );
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

    // Create an actor for the scene in the main 3D view widget.
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper( mapper );
    actor->GetProperty()->EdgeVisibilityOff();

    /** Section is not actually a data file.  It is a composition of two data files (see docs). */
    return View3DViewData( section->getCartesianGrid(), threshold->GetOutput(), actor );
}

View3DViewData View3DBuilders::buildForAttribute2DContourLines(CartesianGrid *cartesianGrid,
                                                               Attribute *attribute,
                                                               View3DWidget *widget3D)
{
    Q_UNUSED( widget3D )

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
    int nZ = 1;
    double X0 = cartesianGrid->getX0();
    double Y0 = cartesianGrid->getY0();
    double Z0 = cartesianGrid->getZ0();
    double dX = cartesianGrid->getDX() + cartesianGrid->getDX() / nX;
    double dY = cartesianGrid->getDY() + cartesianGrid->getDY() / nY;
    double dZ = cartesianGrid->getDZ() + cartesianGrid->getDZ() / nZ;
    double X0frame = X0 - dX/2.0;
    double Y0frame = Y0 - dY/2.0;
    double Z0frame = Z0 - dZ/2.0;

    //create a volumetric object with the shape of a regular 2D grid
    vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
    imageData->SetExtent(0, nX-1, 0, nY-1, 0, 0); //extent (indexes) of GammaRay grids start at i=0,j=0,k=0
    imageData->SetSpacing( dX, dY, dZ );
    imageData->SetOrigin( X0frame, Y0frame, Z0frame );
    imageData->AllocateScalars(VTK_DOUBLE, 1); //each cell will contain one double value.
    int* extent = imageData->GetExtent();

    //populate the volumetric object with the grid values for contour computation
    for (int j = extent[2]; j <= extent[3]; ++j){
        for (int i = extent[0]; i <= extent[1]; ++i){
            double* pixel = static_cast<double*>(imageData->GetScalarPointer( i, j, 0 ));
            pixel[0] = cartesianGrid->dataIJK( var_index - 1, i, j, 0 );
        }
    }

    //Create a filter (algorithm) to convert the 2D grid into the contour polylines.
    vtkSmartPointer<vtkContourFilter> contourFilter = vtkSmartPointer<vtkContourFilter>::New();
    contourFilter->SetInputData( imageData );
    contourFilter->GenerateValues( 20, min, max); // (numContours, rangeStart, rangeEnd)
    contourFilter->Update();

    // Create a VTK mapper and actor to enable visualization
    vtkSmartPointer<vtkPolyDataMapper> mapper =
      vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(contourFilter->GetOutputPort());
    //tubeMapper->SetLookupTable(lut);
    //tubeMapper->SetScalarModeToUseCellData();
    //tubeMapper->SetColorModeToMapScalars();
    //tubeMapper->SelectColorArray("values");
    //tubeMapper->SetScalarRange(min, max);
    mapper->SetScalarVisibility(false); //turns off scalar visibility (will use the actor's global color instead)

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1.0, 1.0, 1.0);
    actor->GetProperty()->SetLineWidth(2.0);

    View3DViewData v3dd( cartesianGrid, contourFilter->GetOutput(), actor );
    v3dd.contourFilter = contourFilter;
    return v3dd;
}

View3DViewData View3DBuilders::buildForAttribute3DCGridIJKClippingVolumetric(CartesianGrid *cartesianGrid,
                                                                             Attribute *attribute,
                                                                             View3DWidget *widget3D)
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
    double dX = cartesianGrid->getDX() + cartesianGrid->getDX() / nX;
    double dY = cartesianGrid->getDY() + cartesianGrid->getDY() / nY;
    double dZ = cartesianGrid->getDZ() + cartesianGrid->getDZ() / nZ;
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

    //read sample values
    values->Allocate( nX*nY*nZ );
    visibility->Allocate( nX*nY*nZ );
    for( int k = 0; k < nZ; ++k){
        for( int j = 0; j < nY; ++j){
            for( int i = 0; i < nX; ++i){
                // sample value
                double value = cartesianGrid->dataIJK( var_index - 1, i, j, k );
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

    //create a volumetric object (regular 3D grid)
    vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
    imageData->SetSpacing( dX, dY, dZ );
    imageData->SetOrigin( X0frame, Y0frame, Z0frame );
    imageData->SetDimensions( nX+1, nY+1, nZ+1 ); //values in volumes reside in the vertexes (corner-point grid)

    //assign the grid values to the volume cells
    imageData->GetCellData()->SetScalars( values );
    imageData->GetCellData()->AddArray( visibility );

    // Create mapper (visualization parameters)
    vtkSmartPointer<vtkSmartVolumeMapper> volumeMapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
    volumeMapper->SetBlendModeToComposite(); // composite first
    volumeMapper->SetInputData( imageData );
    //volumeMapper->SetRequestedRenderModeToRayCast();
    volumeMapper->Update();

    //create a color transfer function (treats categorical variables as continuous values)
    vtkSmartPointer<vtkColorTransferFunction> ctf;
    if( attribute->isCategorical() )
        ctf = View3dColorTables::getCategoricalColorTransferFunction
                ( cartesianGrid->getCategoryDefinition( attribute ), false );
    else
        ctf = View3dColorTables::getColorTransferFunction( ColorTable::RAINBOW, min, max );

    //create transfer mapping scalar value to opacity so unvalued cells are invisible
    //if the user set a no-data value.
    vtkSmartPointer<vtkPiecewiseFunction> otf = vtkSmartPointer<vtkPiecewiseFunction>::New();
    if( cartesianGrid->hasNoDataValue() )
        otf->AddPoint( cartesianGrid->getNoDataValueAsDouble(), 0.0 );
    otf->AddPoint( min, 1.0 );
    otf->AddPoint( max, 1.0 );

    //this object instructs VTK on how to render the volume.
    vtkSmartPointer<vtkVolumeProperty> volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    volumeProperty->ShadeOff();
    volumeProperty->SetColor(ctf);
    volumeProperty->SetScalarOpacity(otf);
    //volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
    volumeProperty->SetInterpolationType(VTK_NEAREST_INTERPOLATION);

    // Finally, pass everything to the vtkVolume (some kind of actor) and return it.
    vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper( volumeMapper );
    volume->SetProperty( volumeProperty );

    return View3DViewData( cartesianGrid, imageData, volume, vtkSmartPointer<vtkThreshold>::New(), volumeMapper );
}

vtkSmartPointer<vtkStructuredGrid> View3DBuilders::makeSurfaceFromSection(Section *section)
{
    vtkSmartPointer<vtkStructuredGrid> structuredGrid = vtkSmartPointer<vtkStructuredGrid>::New();
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

    // Get the Section's component data files.
    PointSet* sectionPathPS = section->getPointSet();
    CartesianGrid* sectionDataCG = section->getCartesianGrid();

    // Sanity checks.
    if( ! sectionPathPS ){
        Application::instance()->logError("View3DBuilders::makeSurfaceFromSection(): Section without a point set "
                                          "defining its geographic path.", true);
        return nullptr;
    }
    if( ! sectionDataCG ){
        Application::instance()->logError("View3DBuilders::makeSurfaceFromSection(): Section without a Cartesian grid "
                                          "containing its data.", true);
        return nullptr;
    }

    // Load the data files.
    sectionPathPS->loadData();
    sectionDataCG->loadData();

    // Get geometry data.
    int nCols = sectionDataCG->getNI();
    int nRows = sectionDataCG->getNK(); //number of rows of a section is the number of layers of its data grid.
    int nHorizons = nRows + 1;
    int nSegments = sectionPathPS->getDataLineCount()-1; //if the PS has two entries, it defines one section segment.

    // Sanity check.
    if( nSegments < 1 ){
        Application::instance()->logError("View3DBuilders::makeSurfaceFromSection(): Section must have at least one segment.", true);
        return nullptr;
    }

    // For each horizon (from bottom of the "fence" to the top).
    for( int iHorizon = 0; iHorizon < nHorizons; ++iHorizon ){

        // For each section segment.
        for( int iSegment = 0;  iSegment < nSegments; ++iSegment){

            //See documentation of the Section class for point set convention regarding variable order.
            double segment_Xi    = sectionPathPS->data( iSegment,   0 ); //X is 1st variable per the section format
            double segment_Yi    = sectionPathPS->data( iSegment,   1 ); //Y is 2nd variable per the section format
            double segment_topZi = sectionPathPS->data( iSegment,   2 ); //Z1 is 3rd variable per the section format
            double segment_botZi = sectionPathPS->data( iSegment,   3 ); //Z2 is 4th variable per the section format
            int dataColIni       = sectionPathPS->data( iSegment,   4 ); //data grid column is 5th variable per the section format
            double segment_Xf    = sectionPathPS->data( iSegment+1, 0 );
            double segment_Yf    = sectionPathPS->data( iSegment+1, 1 );
            double segment_topZf = sectionPathPS->data( iSegment+1, 2 );
            double segment_botZf = sectionPathPS->data( iSegment+1, 3 );
            int dataColFin       = sectionPathPS->data( iSegment+1, 4 ) - 1; //data grid column is 5th variable per the section format
            if( iSegment + 1 == nSegments ) //the last segment's final column is not the first column of the next segment (it can't be!)
                ++dataColFin;
            int nDataColumnsOfSegment = dataColFin - dataColIni + 1;
            int nColumnDividersOfSegment = nDataColumnsOfSegment + 1;

            //For each column divider in the segment
            for( int iColumnDivider = 0; iColumnDivider < nColumnDividersOfSegment; ++iColumnDivider ){

                //The last vertex of the previous segment is the first vertex
                //of the current vertex.  That is, only the first segment defines
                //its first vertex.  For the other segments, this step is skipped.
                if( iSegment != 0 && iColumnDivider == 0 )
                    continue;

                //The deltas of X,Y coordinate are simple because the geologic section is vertical.
                double segmentDeltaX = segment_Xf - segment_Xi;
                double segmentDeltaY = segment_Yf - segment_Yi;

                //The delta z is bit more complicated because depth can vary along the section.
                //Furthermore, we have a bottom and a top z in the head and tail of the section segment.
                double segmentHeadDeltaZ = segment_topZi - segment_botZi;
                double segmentTailDeltaZ = segment_topZf - segment_botZf;
                double segmentHeadZ = segment_botZi + segmentHeadDeltaZ / nHorizons * iHorizon;
                double segmentTailZ = segment_botZf + segmentTailDeltaZ / nHorizons * iHorizon;
                double segmentDeltaZ = segmentTailZ - segmentHeadZ;

                //Compute the vertex location in space and adds it to the collection.
                double x = segment_Xi + segmentDeltaX / nDataColumnsOfSegment * iColumnDivider;
                double y = segment_Yi + segmentDeltaY / nDataColumnsOfSegment * iColumnDivider;
                double z = segmentHeadZ + segmentDeltaZ / nDataColumnsOfSegment * iColumnDivider;
                points->InsertNextPoint(x, y, z);

            } // For each column divider in the segment
        } // For each section segment
    } // For each horizon

    // Specify the dimensions of the grid.
    structuredGrid->SetDimensions(nCols+1, nRows+1, 1);
    structuredGrid->SetPoints(points);

    return structuredGrid;
}
