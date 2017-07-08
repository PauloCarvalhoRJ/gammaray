#include "v3dcfgwidforattributein3dcartesiangrid.h"
#include "ui_v3dcfgwidforattributein3dcartesiangrid.h"

#include "domain/cartesiangrid.h"
#include "domain/application.h"
#include <vtkAlgorithmOutput.h>
#include <vtkInformation.h>
#include <vtkStreamingDemandDrivenPipeline.h>

V3DCfgWidForAttributeIn3DCartesianGrid::V3DCfgWidForAttributeIn3DCartesianGrid(
        CartesianGrid *cartesianGrid,
        Attribute */*attribute*/,
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

    int nXsub = cartesianGrid->getNX() / _viewObjects.samplingRate + 1;
    int nYsub = cartesianGrid->getNY() / _viewObjects.samplingRate + 1;
    int nZsub = cartesianGrid->getNZ() / _viewObjects.samplingRate + 1;
    if( _viewObjects.samplingRate == 1 ){
        nXsub = cartesianGrid->getNX();
        nYsub = cartesianGrid->getNY();
        nZsub = cartesianGrid->getNZ();
    }

    ui->sldILowClip->setMinimum( 0 );
    ui->sldILowClip->setMaximum( nXsub );
    ui->sldILowClip->setValue(0);
    ui->sldIHighClip->setMinimum( 0 );
    ui->sldIHighClip->setMaximum( nXsub );
    ui->sldIHighClip->setValue( nXsub );

    ui->sldJLowClip->setMinimum( 0 );
    ui->sldJLowClip->setMaximum( nYsub );
    ui->sldJLowClip->setValue(0);
    ui->sldJHighClip->setMinimum( 0 );
    ui->sldJHighClip->setMaximum( nYsub );
    ui->sldJHighClip->setValue( nYsub );

    ui->sldKLowClip->setMinimum( 0 );
    ui->sldKLowClip->setMaximum( nZsub );
    ui->sldKLowClip->setValue(0);
    ui->sldKHighClip->setMinimum( 0 );
    ui->sldKHighClip->setMaximum( nZsub );
    ui->sldKHighClip->setValue( nZsub );

    updateLabels();

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
    //assumes a vtkStructuredGridClip and a vtkDataSetMapper exist in the View3DViewData object
    vtkSmartPointer<vtkExtractGrid> subgrider = _viewObjects.subgrider;
    vtkSmartPointer<vtkDataSetMapper> mapper = _viewObjects.mapper;

    //TODO: setting SetOutputWholeExtent alone rises a VTK error, which I didn't find a way
    //      to avoid yet.  So I suppress the warning window, which is not good, but so far
    //      I haven't got a way to work the grid clipping without this error.
    //      I've tried the code further down, for no avail.  I've also tried to clip with some other
    //      function, but none results in clipping.  Documentation of these functions/classes is insufficient.
    //      I also couldn't find an example.  I'll keep trial-and-error in the future.
    vtkObject::GlobalWarningDisplayOff();
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    subgrider->SetVOI(ui->sldILowClip->value(),
                       ui->sldIHighClip->value(),
                       ui->sldJLowClip->value(),
                       ui->sldJHighClip->value(),
                       ui->sldKLowClip->value(),
                       ui->sldKHighClip->value());

    mapper->SetInputConnection( subgrider->GetOutputPort());

    updateLabels();

//    subgrider->UpdateWholeExtent();
//    subgrider->SetUpdateExtentToWholeExtent();
//    int ext[]={0,5,0,5,0,5};
//    subgrider->SetUpdateExtent(ext);
//    subgrider->PropagateUpdateExtent();
//    subgrider->UpdateInformation();
//    subgrider->UpdateDataObject();
//    subgrider->Update();
//    subgrider->Set

    emit changed();
}

void V3DCfgWidForAttributeIn3DCartesianGrid::updateLabels()
{
    //the labels should show the IJK clipping indexes from the original size
    //not of the subsampled cube.  This helps the user in locating an original slice
    //for further analysis

    ui->lblIlowClip->setText ( QString::number( ui->sldILowClip->value() * _viewObjects.samplingRate ) );
    ui->lblIhighClip->setText( QString::number( ui->sldIHighClip->value() * _viewObjects.samplingRate ) );
    ui->lblJlowClip->setText ( QString::number( ui->sldJLowClip->value() * _viewObjects.samplingRate ) );
    ui->lblJhighClip->setText( QString::number( ui->sldJHighClip->value() * _viewObjects.samplingRate ) );
    ui->lblKlowClip->setText ( QString::number( ui->sldKLowClip->value() * _viewObjects.samplingRate ) );
    ui->lblKhighClip->setText( QString::number( ui->sldKHighClip->value() * _viewObjects.samplingRate ) );

    if( _viewObjects.samplingRate > 1 ){
        ui->lblIlowClip->setText ( "~ " + ui->lblIlowClip->text() );
        ui->lblIhighClip->setText( "~ " + ui->lblIhighClip->text() );
        ui->lblJlowClip->setText ( "~ " + ui->lblJlowClip->text() );
        ui->lblJhighClip->setText( "~ " + ui->lblJhighClip->text() );
        ui->lblKlowClip->setText ( "~ " + ui->lblKlowClip->text() );
        ui->lblKhighClip->setText( "~ " + ui->lblKhighClip->text() );
    }
}
