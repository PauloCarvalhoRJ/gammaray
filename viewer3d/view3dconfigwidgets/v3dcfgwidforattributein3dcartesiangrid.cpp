#include "v3dcfgwidforattributein3dcartesiangrid.h"
#include "ui_v3dcfgwidforattributein3dcartesiangrid.h"

#include "domain/cartesiangrid.h"
#include "domain/application.h"
#include <vtkAlgorithmOutput.h>
#include <vtkInformation.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkExtractGrid.h>

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

    //get the object that triggered the call to this slot
    QObject* obj = sender();
    //check which slider was changed by the user
    QSlider* slider = qobject_cast<QSlider*>( obj );
    //validity direction
    bool highPushesLow = true;
    if( slider == ui->sldILowClip || slider == ui->sldJLowClip || slider == ui->sldKLowClip )
        highPushesLow = false;

    //assure validity (e.g. low clip > high clip)
    //adjust slides dynamically depending on which direction the user is adjusting (high pushes low or low pushes high)
    if( ui->sldILowClip->value() > ui->sldIHighClip->value() ){
        if( highPushesLow )
            ui->sldILowClip->setValue( ui->sldIHighClip->value() );
        else
            ui->sldIHighClip->setValue( ui->sldILowClip->value() );
    }
    if( ui->sldJLowClip->value() > ui->sldJHighClip->value() ){
        if( highPushesLow )
            ui->sldJLowClip->setValue( ui->sldJHighClip->value() );
        else
            ui->sldJHighClip->setValue( ui->sldJLowClip->value() );
    }
    if( ui->sldKLowClip->value() > ui->sldKHighClip->value() ){
        if( highPushesLow )
            ui->sldKLowClip->setValue( ui->sldKHighClip->value() );
        else
            ui->sldKHighClip->setValue( ui->sldKLowClip->value() );
    }

    //set the cliping planes
    subgrider->SetVOI( ui->sldILowClip->value(),
                       ui->sldIHighClip->value(),
                       ui->sldJLowClip->value(),
                       ui->sldJHighClip->value(),
                       ui->sldKLowClip->value(),
                       ui->sldKHighClip->value());
    subgrider->Update();

    //update the GUI label readout.
    updateLabels();

    //notify any listeners of changes
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
