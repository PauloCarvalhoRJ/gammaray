#include "v3dcfgwidforattributein3dcartesiangrid.h"
#include "ui_v3dcfgwidforattributein3dcartesiangrid.h"

#include "domain/cartesiangrid.h"
#include "domain/application.h"
#include <vtkAlgorithmOutput.h>
#include <vtkInformation.h>
#include <vtkStreamingDemandDrivenPipeline.h>

V3DCfgWidForAttributeIn3DCartesianGrid::V3DCfgWidForAttributeIn3DCartesianGrid(
        CartesianGrid *cartesianGrid,
        Attribute *attribute,
        View3DViewData viewObjects,
        QWidget *parent) :
    View3DConfigWidget(parent),
    ui(new Ui::V3DCfgWidForAttributeIn3DCartesianGrid),
    _viewObjects( viewObjects )
{
    ui->setupUi(this);

    //prevent signals from being fired while configuring the sliders
    ui->sldILowClip->blockSignals(true);
    ui->sldIHighClip->blockSignals(true);
    ui->sldJLowClip->blockSignals(true);
    ui->sldJHighClip->blockSignals(true);
    ui->sldKLowClip->blockSignals(true);
    ui->sldKHighClip->blockSignals(true);

    ui->sldILowClip->setMinimum( 0 );
    ui->sldILowClip->setMaximum( cartesianGrid->getNX() );
    ui->sldILowClip->setValue(0);
    ui->sldIHighClip->setMinimum( 0 );
    ui->sldIHighClip->setMaximum( cartesianGrid->getNX() );
    ui->sldIHighClip->setValue(cartesianGrid->getNX());

    ui->sldJLowClip->setMinimum( 0 );
    ui->sldJLowClip->setMaximum( cartesianGrid->getNY() );
    ui->sldJLowClip->setValue(0);
    ui->sldJHighClip->setMinimum( 0 );
    ui->sldJHighClip->setMaximum( cartesianGrid->getNY() );
    ui->sldJHighClip->setValue(cartesianGrid->getNY());

    ui->sldKLowClip->setMinimum( 0 );
    ui->sldKLowClip->setMaximum( cartesianGrid->getNZ() );
    ui->sldKLowClip->setValue(0);
    ui->sldKHighClip->setMinimum( 0 );
    ui->sldKHighClip->setMaximum( cartesianGrid->getNZ() );
    ui->sldKHighClip->setValue(cartesianGrid->getNZ());

    //restore signal processing
    ui->sldILowClip->blockSignals(false);
    ui->sldIHighClip->blockSignals(false);
    ui->sldJLowClip->blockSignals(false);
    ui->sldJHighClip->blockSignals(false);
    ui->sldKLowClip->blockSignals(false);
    ui->sldKHighClip->blockSignals(false);
}

V3DCfgWidForAttributeIn3DCartesianGrid::~V3DCfgWidForAttributeIn3DCartesianGrid()
{
    delete ui;
}

void V3DCfgWidForAttributeIn3DCartesianGrid::onUserMadeChanges()
{

    //Since we are in a V3DCfgWidForAttributeIn3DCartesianGrid (data cube with clipping)
    //assumes a vtkStructuredGridClip exists in the View3DViewData object
    vtkSmartPointer<vtkStructuredGridClip> clipper = _viewObjects.clipper;

    //TODO: setting SetOutputWholeExtent alone rises a VTK error, which I didn't find a way
    //      to avoid yet.  So I suppress the warning window, which is not good, but so far
    //      I haven't got a way to work the grid clipping without this error.
    //      I've tried the code further down, for no avail.  I've also tried to clip with some other
    //      function, but none results in clipping.  Documentation of these functions/classes is insufficient.
    //      I also couldn't find an example.  I'll keep trial-and-error in the future.
    vtkObject::GlobalWarningDisplayOff();
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    clipper->SetOutputWholeExtent( ui->sldILowClip->value(),
                                   ui->sldIHighClip->value(),
                                   ui->sldJLowClip->value(),
                                   ui->sldJHighClip->value(),
                                   ui->sldKLowClip->value(),
                                   ui->sldKHighClip->value() );
//    clipper->UpdateWholeExtent();
//    clipper->SetUpdateExtentToWholeExtent();
//    int ext[]={0,5,0,5,0,5};
//    clipper->SetUpdateExtent(ext);
//    clipper->PropagateUpdateExtent();
//    clipper->UpdateInformation();
//    clipper->UpdateDataObject();
//    clipper->Update();

    emit changed();
}
