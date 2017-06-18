#include "view3dviewdata.h"

#include <vtkActor.h>

View3DViewData::View3DViewData() :
    actor( vtkSmartPointer<vtkActor>::New() ),
    clipper( vtkSmartPointer<vtkStructuredGridClip>::New() )
{}

View3DViewData::View3DViewData(vtkSmartPointer<vtkProp> pActor) :
    actor( pActor ),
    clipper( vtkSmartPointer<vtkStructuredGridClip>::New() )
{}

View3DViewData::View3DViewData(vtkSmartPointer<vtkProp> pActor,
                               vtkSmartPointer<vtkStructuredGridClip> pClipper) :
    actor( pActor ),
    clipper( pClipper )
{}
