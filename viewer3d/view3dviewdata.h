#ifndef VIEW3DVIEWDATA_H
#define VIEW3DVIEWDATA_H

#include <vtkSmartPointer.h>

class vtkProp;
class vtkStructuredGridClip;
class vtkExtractGrid;
class vtkDataSetMapper;
class vtkThreshold;
class vtkTubeFilter;
class vtkBillboardTextActor3D;
class vtkAbstractVolumeMapper;

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
                   vtkSmartPointer<vtkDataSetMapper> pMapper,
                   vtkSmartPointer<vtkThreshold> pThreshold,
                   int sRate = 1);

    View3DViewData(vtkSmartPointer<vtkProp> pActor,
                   vtkSmartPointer<vtkThreshold> pThreshold);

    View3DViewData(vtkSmartPointer<vtkProp> pActor,
                   vtkSmartPointer<vtkDataSetMapper> pMapper,
                   vtkSmartPointer<vtkThreshold> pThreshold);

    View3DViewData(vtkSmartPointer<vtkProp> pActor,
                   vtkSmartPointer<vtkDataSetMapper> pMapper);

    View3DViewData(vtkSmartPointer<vtkProp> pActor,
                   vtkSmartPointer<vtkThreshold> pThreshold,
                   vtkSmartPointer<vtkAbstractVolumeMapper> pVolumeMapper
                   );

    /** All objects must have a VTKActor to become visible. */
    vtkSmartPointer<vtkProp> actor;

    /** Some objects may have a configurable model clipper (cutting plane). */
    vtkSmartPointer<vtkStructuredGridClip> clipper;

    /** Some objects may have a configurable sub-grider/grid resampler. */
    vtkSmartPointer<vtkExtractGrid> subgrider;

    /** Some objects may have a configurable data set mapper. */
    vtkSmartPointer<vtkDataSetMapper> mapper;

    /** The thresolhder may be needed to hide unvalued data locations. */
    vtkSmartPointer<vtkThreshold> threshold;

    /** The tube filter is used to render segment sets with adjustable diameter. */
    vtkSmartPointer<vtkTubeFilter> tubeFilter;

    /** An object may provide a VTKActor with its label text. */
    vtkSmartPointer<vtkBillboardTextActor3D> labelActor;

    /** Some objects may have a configurable volume mapper. */
    vtkSmartPointer<vtkAbstractVolumeMapper> volumeMapper;

    /** Sampling rate. Default is 1: 1 cell per 1 sample in each topological direction (I, J, K). */
    int samplingRate;
};

#endif // VIEW3DVIEWDATA_H
