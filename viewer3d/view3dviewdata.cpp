#include "view3dviewdata.h"

#include <vtkActor.h>
#include <vtkProp.h>
#include <vtkStructuredGridClip.h>
#include <vtkExtractGrid.h>
#include <vtkDataSetMapper.h>
#include <vtkThreshold.h>

View3DViewData::View3DViewData() :
    actor( vtkSmartPointer<vtkActor>::New() ),
    clipper( vtkSmartPointer<vtkStructuredGridClip>::New() ),
    subgrider( vtkSmartPointer<vtkExtractGrid>::New() ),
    mapper( vtkSmartPointer<vtkDataSetMapper>::New() ),
    threshold( vtkSmartPointer<vtkThreshold>::New() ),
    samplingRate( 1 )
{}

View3DViewData::View3DViewData(vtkSmartPointer<vtkProp> pActor) :
    actor( pActor ),
    clipper( vtkSmartPointer<vtkStructuredGridClip>::New() ),
    subgrider( vtkSmartPointer<vtkExtractGrid>::New() ),
    mapper( vtkSmartPointer<vtkDataSetMapper>::New() ),
    threshold( vtkSmartPointer<vtkThreshold>::New() ),
    samplingRate( 1 )
{}

View3DViewData::View3DViewData(vtkSmartPointer<vtkProp> pActor,
                               vtkSmartPointer<vtkStructuredGridClip> pClipper) :
    actor( pActor ),
    clipper( pClipper ),
    subgrider( vtkSmartPointer<vtkExtractGrid>::New() ),
    mapper( vtkSmartPointer<vtkDataSetMapper>::New() ),
    threshold( vtkSmartPointer<vtkThreshold>::New() ),
    samplingRate( 1 )
{}

View3DViewData::View3DViewData(vtkSmartPointer<vtkProp> pActor, vtkSmartPointer<vtkExtractGrid> pSubgrider) :
    actor( pActor ),
    clipper( vtkSmartPointer<vtkStructuredGridClip>::New() ),
    subgrider( pSubgrider ),
    mapper( vtkSmartPointer<vtkDataSetMapper>::New() ),
    threshold( vtkSmartPointer<vtkThreshold>::New() ),
    samplingRate( 1 )
{}

View3DViewData::View3DViewData(vtkSmartPointer<vtkProp> pActor,
                               vtkSmartPointer<vtkExtractGrid> pSubgrider,
                               vtkSmartPointer<vtkDataSetMapper> pMapper,
                               vtkSmartPointer<vtkThreshold> pThreshold,
                               int sRate):
    actor( pActor ),
    clipper( vtkSmartPointer<vtkStructuredGridClip>::New() ),
    subgrider( pSubgrider ),
    mapper( pMapper ),
    threshold( pThreshold ),
    samplingRate( sRate )
{}

View3DViewData::View3DViewData(vtkSmartPointer<vtkProp> pActor, vtkSmartPointer<vtkThreshold> pThreshold) :
    actor( pActor ),
    clipper( vtkSmartPointer<vtkStructuredGridClip>::New() ),
    subgrider( vtkSmartPointer<vtkExtractGrid>::New() ),
    mapper( vtkSmartPointer<vtkDataSetMapper>::New() ),
    threshold( pThreshold ),
    samplingRate( 1 )
{
}
