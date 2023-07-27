#ifndef VIEW3DVIEWDATA_H
#define VIEW3DVIEWDATA_H

#include <vtkSmartPointer.h>
#include <vector>

class vtkProp;
class vtkStructuredGridClip;
class vtkExtractGrid;
class vtkDataSetMapper;
class vtkThreshold;
class vtkTubeFilter;
class vtkBillboardTextActor3D;
class vtkAbstractVolumeMapper;
class vtkContourFilter;
class vtkDataSet;

class DataFile; //from /domain source subdirectory

/** This class is just a data structure to hold objects and info related to 3D visualization of a domain object.
 * E.g.: the vtkActor built for it.
 */
class View3DViewData
{
public:
    View3DViewData();

    View3DViewData(DataFile* originalDataFile,
                   vtkSmartPointer<vtkDataSet> actualDataSet,
                   vtkSmartPointer<vtkProp> pActor);

    View3DViewData(DataFile *originalDataFile,
                   vtkSmartPointer<vtkDataSet> actualDataSet,
                   vtkSmartPointer<vtkProp> pActor,
                   vtkSmartPointer<vtkStructuredGridClip> pClipper);

    View3DViewData(DataFile *originalDataFile,
                   vtkSmartPointer<vtkDataSet> actualDataSet,
                   vtkSmartPointer<vtkProp> pActor,
                   vtkSmartPointer<vtkExtractGrid> pSubgrider);

    View3DViewData(DataFile *originalDataFile,
                   vtkSmartPointer<vtkDataSet> actualDataSet,
                   vtkSmartPointer<vtkProp> pActor,
                   vtkSmartPointer<vtkExtractGrid> pSubgrider,
                   vtkSmartPointer<vtkDataSetMapper> pMapper,
                   vtkSmartPointer<vtkThreshold> pThreshold,
                   int sRate = 1);

    View3DViewData(DataFile *originalDataFile,
                   vtkSmartPointer<vtkDataSet> actualDataSet,
                   vtkSmartPointer<vtkProp> pActor,
                   vtkSmartPointer<vtkThreshold> pThreshold);

    View3DViewData(DataFile *originalDataFile,
                   vtkSmartPointer<vtkDataSet> actualDataSet,
                   vtkSmartPointer<vtkProp> pActor,
                   vtkSmartPointer<vtkDataSetMapper> pMapper,
                   vtkSmartPointer<vtkThreshold> pThreshold);

    View3DViewData(DataFile *originalDataFile,
                   vtkSmartPointer<vtkDataSet> actualDataSet,
                   vtkSmartPointer<vtkProp> pActor,
                   vtkSmartPointer<vtkDataSetMapper> pMapper);

    View3DViewData(DataFile *originalDataFile,
                   vtkSmartPointer<vtkDataSet> actualDataSet,
                   vtkSmartPointer<vtkProp> pActor,
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

    /** The contour filter is used to render 2D or 3D grids as contours or isosurfaces. */
    vtkSmartPointer<vtkContourFilter> contourFilter;

    /** An object may provide a VTKActor with its label text. */
    vtkSmartPointer<vtkBillboardTextActor3D> labelActor;

    /** Some objects may have a configurable volume mapper. */
    vtkSmartPointer<vtkAbstractVolumeMapper> volumeMapper;

    /** An object may have randomly placed captions (e.g. category names, descriptions, notes, etc.). */
    std::vector< vtkSmartPointer<vtkBillboardTextActor3D> > captionActors;

    /** The pointer to the underlying vtkDataSet.
     * This should be the vtkDataSet actually passed to the vtkMapper passed to the actor, which is not always
     * necessarily the original vtkDataSet originally built from the file data.  For example, the correct pointer
     * should be the one returned by the GetOutput() method of the last filter down in the builder pipeline.
     */
    vtkSmartPointer<vtkDataSet> actualDataSet;

    /** The pointer to the original domain data file used to build the visual objects refered in this object. */
    DataFile* originalDataFile;

    /** Sampling rate. Default is 1: 1 cell per 1 sample in each topological direction (I, J, K). */
    int samplingRate;
};

#endif // VIEW3DVIEWDATA_H
