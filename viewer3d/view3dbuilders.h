#ifndef VIEW3DBUILDERS_H
#define VIEW3DBUILDERS_H

#include <vtkSmartPointer.h>
#include <vtkActor.h>

class ProjectComponent;
class PointSet;
class Attribute;
class CartesianGrid;

/**
 * This class groups static functions to build VTK actors for the several domain objects.
 * In other words, if you need an object visible in the 3D Viewer, then you need to create a build() function
 * for the object's class.  You also need to implement the ProjectComponent::buildVTKActor() in the target domain class.
 */
class View3DBuilders
{
public:
    View3DBuilders();

    /** Generic builder (fallback implementation). */
    static vtkSmartPointer<vtkActor> build(ProjectComponent *object );

    //@{
    /** Specific overrides. */
    static vtkSmartPointer<vtkActor> build( PointSet* object ); //point set geometry only
    static vtkSmartPointer<vtkActor> build( Attribute* object ); //attribute (can be of point set, cartesian grid, etc.)
    static vtkSmartPointer<vtkActor> build( CartesianGrid* object );  //cartesian grid geometry only
    //@}

private:
    static vtkSmartPointer<vtkActor> buildForAttributeFromPointSet( PointSet* pointSet, Attribute* attribute );

    /** Specific builder for a Cartesian grid that represents a 2D map (nZ < 2).
     *  The grid is displayed in the XY plane (Z=0).
    */
    static vtkSmartPointer<vtkActor> buildForMapCartesianGrid( CartesianGrid* cartesianGrid );
};

#endif // VIEW3DBUILDERS_H
