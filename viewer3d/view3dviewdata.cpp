#include "view3dviewdata.h"

#include <vtkActor.h>
#include <vtkProp.h>
#include <vtkStructuredGridClip.h>
#include <vtkExtractGrid.h>
#include <vtkDataSetMapper.h>
#include <vtkThreshold.h>
#include <vtkSmartVolumeMapper.h>

View3DViewData::View3DViewData() :
    samplingRate( 1 )
{}

View3DViewData::View3DViewData(vtkSmartPointer<vtkProp> pActor) :
    actor( pActor ),
    samplingRate( 1 )
{}

View3DViewData::View3DViewData(vtkSmartPointer<vtkProp> pActor,
                               vtkSmartPointer<vtkStructuredGridClip> pClipper) :
    actor( pActor ),
    clipper( pClipper ),
    samplingRate( 1 )
{}

View3DViewData::View3DViewData(vtkSmartPointer<vtkProp> pActor, vtkSmartPointer<vtkExtractGrid> pSubgrider) :
    actor( pActor ),
    subgrider( pSubgrider ),
    samplingRate( 1 )
{}

View3DViewData::View3DViewData(vtkSmartPointer<vtkProp> pActor,
                               vtkSmartPointer<vtkExtractGrid> pSubgrider,
                               vtkSmartPointer<vtkDataSetMapper> pMapper,
                               vtkSmartPointer<vtkThreshold> pThreshold,
                               int sRate):
    actor( pActor ),
    subgrider( pSubgrider ),
    mapper( pMapper ),
    threshold( pThreshold ),
    samplingRate( sRate )
{}

View3DViewData::View3DViewData(vtkSmartPointer<vtkProp> pActor, vtkSmartPointer<vtkThreshold> pThreshold) :
    actor( pActor ),
    threshold( pThreshold ),
    samplingRate( 1 )
{
}

View3DViewData::View3DViewData(vtkSmartPointer<vtkProp> pActor,
                               vtkSmartPointer<vtkDataSetMapper> pMapper,
                               vtkSmartPointer<vtkThreshold> pThreshold) :
    actor( pActor ),
    mapper( pMapper ),
    threshold( pThreshold ),
    samplingRate( 1 )
{

}

View3DViewData::View3DViewData(vtkSmartPointer<vtkProp> pActor, vtkSmartPointer<vtkDataSetMapper> pMapper) :
actor( pActor ),
mapper( pMapper ),
samplingRate( 1 )
{

}

View3DViewData::View3DViewData(vtkSmartPointer<vtkProp> pActor,
                               vtkSmartPointer<vtkThreshold> pThreshold,
                               vtkSmartPointer<vtkAbstractVolumeMapper> pVolumeMapper) :
    actor( pActor ),
    threshold( pThreshold ),
    volumeMapper( pVolumeMapper ),
    samplingRate( 1 )
{
}
