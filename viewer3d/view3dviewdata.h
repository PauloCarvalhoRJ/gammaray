#ifndef VIEW3DVIEWDATA_H
#define VIEW3DVIEWDATA_H

#include <vtkSmartPointer.h>
#include <vtkProp.h>
#include <vtkStructuredGridClip.h>

/** This class is just a data structure to hold objects and info related to 3D visualization of a domain object.
 * E.g.: the vtkActor built for it.
 */
class View3DViewData
{
public:
    View3DViewData();
    View3DViewData(vtkSmartPointer<vtkProp> pActor);
    View3DViewData(vtkSmartPointer<vtkProp> pActor,
                   vtkSmartPointer<vtkStructuredGridClip> pClipper);


    /** All objects must have a VTKActor to become visible. */
    vtkSmartPointer<vtkProp> actor;

    /** Some objects may have a configurable model clipper (cutting plane). */
    vtkSmartPointer<vtkStructuredGridClip> clipper;
};

#endif // VIEW3DVIEWDATA_H
