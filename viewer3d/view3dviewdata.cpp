#include "view3dviewdata.h"

#include <vtkActor.h>

View3DViewData::View3DViewData() :
    actor( vtkSmartPointer<vtkActor>::New() ),
    clipper( vtkSmartPointer<vtkStructuredGridClip>::New() ),
    subgrider( vtkSmartPointer<vtkExtractGrid>::New() ),
    mapper( vtkSmartPointer<vtkDataSetMapper>::New() ),
    samplingRate( 1 )
{}

View3DViewData::View3DViewData(vtkSmartPointer<vtkProp> pActor) :
    actor( pActor ),
    clipper( vtkSmartPointer<vtkStructuredGridClip>::New() ),
    subgrider( vtkSmartPointer<vtkExtractGrid>::New() ),
    mapper( vtkSmartPointer<vtkDataSetMapper>::New() ),
    samplingRate( 1 )
{}

View3DViewData::View3DViewData(vtkSmartPointer<vtkProp> pActor,
                               vtkSmartPointer<vtkStructuredGridClip> pClipper) :
    actor( pActor ),
    clipper( pClipper ),
    subgrider( vtkSmartPointer<vtkExtractGrid>::New() ),
    mapper( vtkSmartPointer<vtkDataSetMapper>::New() ),
    samplingRate( 1 )
{}

View3DViewData::View3DViewData(vtkSmartPointer<vtkProp> pActor, vtkSmartPointer<vtkExtractGrid> pSubgrider) :
    actor( pActor ),
    clipper( vtkSmartPointer<vtkStructuredGridClip>::New() ),
    subgrider( pSubgrider ),
    mapper( vtkSmartPointer<vtkDataSetMapper>::New() ),
    samplingRate( 1 )
{}

View3DViewData::View3DViewData(vtkSmartPointer<vtkProp> pActor,
                               vtkSmartPointer<vtkExtractGrid> pSubgrider,
                               vtkSmartPointer<vtkDataSetMapper> pMapper,
                               int sRate):
    actor( pActor ),
    clipper( vtkSmartPointer<vtkStructuredGridClip>::New() ),
    subgrider( pSubgrider ),
    mapper( pMapper ),
    samplingRate( sRate )
{}
