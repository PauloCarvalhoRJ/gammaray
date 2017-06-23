#ifndef VIEW3DVIEWDATA_H
#define VIEW3DVIEWDATA_H

#include <vtkSmartPointer.h>
#include <vtkProp.h>
#include <vtkStructuredGridClip.h>
#include <vtkExtractGrid.h>
#include <vtkDataSetMapper.h>

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
    View3DViewData(vtkSmartPointer<vtkProp> pActor,
                   vtkSmartPointer<vtkExtractGrid> pSubgrider);
    View3DViewData(vtkSmartPointer<vtkProp> pActor,
                   vtkSmartPointer<vtkExtractGrid> pSubgrider,
                   vtkSmartPointer<vtkDataSetMapper> pMapper);


    /** All objects must have a VTKActor to become visible. */
    vtkSmartPointer<vtkProp> actor;

    /** Some objects may have a configurable model clipper (cutting plane). */
    vtkSmartPointer<vtkStructuredGridClip> clipper;

    /** Some objects may have a configurable sub-grider/grid resampler. */
    vtkSmartPointer<vtkExtractGrid> subgrider;

    /** Some objects may have a configurable data set mapper. */
    vtkSmartPointer<vtkDataSetMapper> mapper;
};

#endif // VIEW3DVIEWDATA_H
