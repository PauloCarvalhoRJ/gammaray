#ifndef VIEW3DBUILDERS_H
#define VIEW3DBUILDERS_H

#include <vtkSmartPointer.h>
#include <vtkActor.h>

class ProjectComponent;
class PointSet;
class Attribute;

/**
 * This class groups static functions to build VTK actors for the several domain objects.
 */
class View3DBuilders
{
public:
    View3DBuilders();

    /** Generic builder (default implementation). */
    static vtkSmartPointer<vtkActor> build(ProjectComponent *object );

    //@{
    /** Specific overrides. */
    static vtkSmartPointer<vtkActor> build( PointSet* object );
    static vtkSmartPointer<vtkActor> build( Attribute* object );
    //@}

private:
    static vtkSmartPointer<vtkActor> buildForAttributeFromPointSet( PointSet* pointSet, Attribute* attribute );
};

#endif // VIEW3DBUILDERS_H
