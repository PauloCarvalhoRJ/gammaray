#include "view3dbuilders.h"
#include "domain/application.h"
#include "domain/projectcomponent.h"
#include "domain/pointset.h"

#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>

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
    vtkSmartPointer<vtkPolyData> point =
      vtkSmartPointer<vtkPolyData>::New();

    // Set the points and vertices we created as the geometry and topology of the polydata
    point->SetPoints(points);
    point->SetVerts(vertices);

    // Visualize
    vtkSmartPointer<vtkPolyDataMapper> mapper =
      vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(point);

    // Create and configure the actor
    vtkSmartPointer<vtkActor> actor =
      vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetPointSize(3);

    return actor;
}
