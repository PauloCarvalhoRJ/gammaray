#include "v3dcfgwidforattributeinmapcartesiangrid.h"
#include "ui_v3dcfgwidforattributeinmapcartesiangrid.h"

V3DCfgWidForAttributeInMapCartesianGrid::V3DCfgWidForAttributeInMapCartesianGrid(
        CartesianGrid *cartesianGrid,
        Attribute *attribute,
        View3DViewData viewObjects,
        QWidget *parent) :
    View3DConfigWidget(parent),
    ui(new Ui::V3DCfgWidForAttributeInMapCartesianGrid),
    _viewObjects( viewObjects )
{
    ui->setupUi(this);

    //prevent signals from being fired while configuring the spinner
    ui->spinSamplingRate->blockSignals(true);

    //assumes all three sample rates (for I, J and K directions) are the same
    int * rate = _viewObjects.subgrider->GetSampleRate();
    // TODO: weird... setting sampling rate for a value lass than originally set in pipe builder (serr View3DBuilders)
    //       causes the actor to disappear.  VTK got to get better documentation or examples...
    //       don't know what to do to be able to increase the sample rate.
    ui->spinSamplingRate->setMinimum( rate[0] );

    //restore signal emiting for the spinner
    ui->spinSamplingRate->blockSignals(false);

}

V3DCfgWidForAttributeInMapCartesianGrid::~V3DCfgWidForAttributeInMapCartesianGrid()
{
    delete ui;
}

void V3DCfgWidForAttributeInMapCartesianGrid::onUserMadeChanges()
{
    vtkSmartPointer<vtkExtractGrid> subgrider = _viewObjects.subgrider;

    //TODO: setting SetSampleRate alone rises a VTK error, which I didn't find a way
    //      to avoid yet.  So I suppress the warning window, which is not good, but so far
    //      I haven't got a way to work the grid resampling without this error.
    //      I also couldn't find a more complete example.  I'll keep trial-and-error in the future.
    vtkObject::GlobalWarningDisplayOff();
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    subgrider->SetSampleRate( ui->spinSamplingRate->value(),
                              ui->spinSamplingRate->value(),
                              ui->spinSamplingRate->value() );

    emit changed();
}
