#include "view3dviewdata.h"

#include <vtkActor.h>
#include <vtkProp.h>
#include <vtkStructuredGridClip.h>
#include <vtkExtractGrid.h>
#include <vtkDataSetMapper.h>
#include <vtkThreshold.h>
#include <vtkSmartVolumeMapper.h>

#include "domain/application.h"

View3DViewData::View3DViewData() :
    originalDataFile(nullptr),
    samplingRate( 1 )
{
    Application::instance()->logInfo("Informed source vtkDataSet pointer: " + QString::number( (long long)(actualDataSet.Get()) ));
}

View3DViewData::View3DViewData(DataFile *originalDataFile,
                               vtkSmartPointer<vtkDataSet> actualDataSet,
                               vtkSmartPointer<vtkProp> pActor) :
    actor( pActor ),
    actualDataSet(actualDataSet),
    originalDataFile(originalDataFile),
    samplingRate( 1 )
{
    Application::instance()->logInfo("View3DViewData::View3DViewData(): Informed source vtkDataSet pointer: " + QString::number( (long long)(actualDataSet.Get()) ));
}

View3DViewData::View3DViewData(DataFile *originalDataFile,
                               vtkSmartPointer<vtkDataSet> actualDataSet,
                               vtkSmartPointer<vtkProp> pActor,
                               vtkSmartPointer<vtkStructuredGridClip> pClipper) :
    actor( pActor ),
    clipper( pClipper ),
    actualDataSet(actualDataSet),
    originalDataFile(originalDataFile),
    samplingRate( 1 )
{
    Application::instance()->logInfo("View3DViewData::View3DViewData(): Informed source vtkDataSet pointer: " + QString::number( (long long)(actualDataSet.Get()) ));
}

View3DViewData::View3DViewData(DataFile *originalDataFile,
                               vtkSmartPointer<vtkDataSet> actualDataSet,
                               vtkSmartPointer<vtkProp> pActor,
                               vtkSmartPointer<vtkExtractGrid> pSubgrider) :
    actor( pActor ),
    subgrider( pSubgrider ),
    actualDataSet(actualDataSet),
    originalDataFile(originalDataFile),
    samplingRate( 1 )
{
    Application::instance()->logInfo("View3DViewData::View3DViewData(): Informed source vtkDataSet pointer: " + QString::number( (long long)(actualDataSet.Get()) ));
}

View3DViewData::View3DViewData(DataFile *originalDataFile,
                               vtkSmartPointer<vtkDataSet> actualDataSet,
                               vtkSmartPointer<vtkProp> pActor,
                               vtkSmartPointer<vtkExtractGrid> pSubgrider,
                               vtkSmartPointer<vtkDataSetMapper> pMapper,
                               vtkSmartPointer<vtkThreshold> pThreshold,
                               int sRate):
    actor( pActor ),
    subgrider( pSubgrider ),
    mapper( pMapper ),
    threshold( pThreshold ),
    actualDataSet(actualDataSet),
    originalDataFile(originalDataFile),
    samplingRate( sRate )
{
    Application::instance()->logInfo("View3DViewData::View3DViewData(): Informed source vtkDataSet pointer: " + QString::number( (long long)(actualDataSet.Get()) ));
}

View3DViewData::View3DViewData(DataFile *originalDataFile,
                               vtkSmartPointer<vtkDataSet> actualDataSet,
                               vtkSmartPointer<vtkProp> pActor,
                               vtkSmartPointer<vtkThreshold> pThreshold) :
    actor( pActor ),
    threshold( pThreshold ),
    actualDataSet(actualDataSet),
    originalDataFile(originalDataFile),
    samplingRate( 1 )
{
    Application::instance()->logInfo("View3DViewData::View3DViewData(): Informed source vtkDataSet pointer: " + QString::number( (long long)(actualDataSet.Get()) ));
}

View3DViewData::View3DViewData(DataFile *originalDataFile,
                               vtkSmartPointer<vtkDataSet> actualDataSet,
                               vtkSmartPointer<vtkProp> pActor,
                               vtkSmartPointer<vtkDataSetMapper> pMapper,
                               vtkSmartPointer<vtkThreshold> pThreshold) :
    actor( pActor ),
    mapper( pMapper ),
    threshold( pThreshold ),
    actualDataSet(actualDataSet),
    originalDataFile(originalDataFile),
    samplingRate( 1 )
{
    Application::instance()->logInfo("View3DViewData::View3DViewData(): Informed source vtkDataSet pointer: " + QString::number( (long long)(actualDataSet.Get()) ));
}

View3DViewData::View3DViewData(DataFile *originalDataFile,
                               vtkSmartPointer<vtkDataSet> actualDataSet,
                               vtkSmartPointer<vtkProp> pActor,
                               vtkSmartPointer<vtkDataSetMapper> pMapper) :
   actor( pActor ),
   mapper( pMapper ),
   actualDataSet(actualDataSet),
   originalDataFile(originalDataFile),
   samplingRate( 1 )
{
    Application::instance()->logInfo("View3DViewData::View3DViewData(): Informed source vtkDataSet pointer: " + QString::number( (long long)(actualDataSet.Get()) ));
}

View3DViewData::View3DViewData(DataFile *originalDataFile,
                               vtkSmartPointer<vtkDataSet> actualDataSet,
                               vtkSmartPointer<vtkProp> pActor,
                               vtkSmartPointer<vtkThreshold> pThreshold,
                               vtkSmartPointer<vtkAbstractVolumeMapper> pVolumeMapper) :
    actor( pActor ),
    threshold( pThreshold ),
    volumeMapper( pVolumeMapper ),
    actualDataSet(actualDataSet),
    originalDataFile(originalDataFile),
    samplingRate( 1 )
{
    Application::instance()->logInfo("View3DViewData::View3DViewData(): Informed source vtkDataSet pointer: " + QString::number( (long long)(actualDataSet.Get()) ));
}
