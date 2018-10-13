#include "v3dcfgwidforattributein3dcartesiangrid.h"
#include "ui_v3dcfgwidforattributein3dcartesiangrid.h"

#include "domain/cartesiangrid.h"
#include "domain/application.h"
#include "../view3dbuilders.h" // for the InvisibilityFlag enum
#include <vtkAlgorithmOutput.h>
#include <vtkInformation.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkExtractGrid.h>
#include <vtkUnstructuredGrid.h>
#include <vtkIntArray.h>
#include <vtkThreshold.h>
#include <vtkCellData.h>

V3DCfgWidForAttributeIn3DCartesianGrid::V3DCfgWidForAttributeIn3DCartesianGrid(GridFile *gridFile,
        Attribute */*attribute*/,
        View3DViewData viewObjects,
        QWidget *parent) :
    View3DConfigWidget(parent),
    ui(new Ui::V3DCfgWidForAttributeIn3DCartesianGrid),
    _viewObjects( viewObjects ),
    m_gridFile( gridFile )
{
    ui->setupUi(this);

    //prevent signals from being fired while configuring the sliders
    ui->sldILowClip->blockSignals(true);
    ui->sldIHighClip->blockSignals(true);
    ui->sldJLowClip->blockSignals(true);
    ui->sldJHighClip->blockSignals(true);
    ui->sldKLowClip->blockSignals(true);
    ui->sldKHighClip->blockSignals(true);

    int nXsub = 1;
    int nYsub = 1;
    int nZsub = 1;
    if( gridFile ){
        nXsub = gridFile->getNI() / _viewObjects.samplingRate + 1;
        nYsub = gridFile->getNJ() / _viewObjects.samplingRate + 1;
        nZsub = gridFile->getNK() / _viewObjects.samplingRate + 1;
        if( _viewObjects.samplingRate == 1 ){
            nXsub = gridFile->getNI();
            nYsub = gridFile->getNJ();
            nZsub = gridFile->getNK();
        }
    } else {
        Application::instance()->logError("V3DCfgWidForAttributeIn3DCartesianGrid::V3DCfgWidForAttributeIn3DCartesianGrid(): null grid file.");
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

    if( m_gridFile && m_gridFile->isRegular() ){
        //Since we are in a V3DCfgWidForAttributeIn3DCartesianGrid (data cube with clipping)
        //assumes a vtkStructuredGridClip and a vtkDataSetMapper exist in the View3DViewData object
        vtkSmartPointer<vtkExtractGrid> subgrider = _viewObjects.subgrider;

        //set the cliping planes
        subgrider->SetVOI( ui->sldILowClip->value(),
                           ui->sldIHighClip->value(),
                           ui->sldJLowClip->value(),
                           ui->sldJHighClip->value(),
                           ui->sldKLowClip->value(),
                           ui->sldKHighClip->value());
        subgrider->Update();

    } else if ( m_gridFile ) {
        //Since it is an Attribute of a GeoGrid, get the corresponding VTK object.
        vtkSmartPointer<vtkUnstructuredGrid> unstructuredGrid =
                dynamic_cast<vtkUnstructuredGrid*>(_viewObjects.threshold->GetInputDataObject(0,0));
        if( ! unstructuredGrid ){
            Application::instance()->logError("V3DCfgWidForAttributeIn3DCartesianGrid::onUserMadeChanges(): unstructured grid not found. Check View3DBuilders::buildForAttributeGeoGrid().");
            return;
        }

        //Get visibility array
        vtkSmartPointer<vtkIntArray> visibilityArray =
                dynamic_cast<vtkIntArray*>( unstructuredGrid->GetCellData()->GetArray( "Visibility" ) );
        if( ! visibilityArray ){
            Application::instance()->logError("V3DCfgWidForAttributeIn3DCartesianGrid::onUserMadeChanges(): visibility array not found. Check View3DBuilders::buildForAttributeGeoGrid().");
            return;
        }

        //Set transparency values for those cells outside clippling limits
        // convention:
        // 1 = visible
        // 0 = invisible due to null value
        // -1 = invisible due to being outside IJK clipping limits
        int nI = m_gridFile->getNI();
        int nJ = m_gridFile->getNJ();
        int nK = m_gridFile->getNK();
        for( int k = 0; k < nK; ++k )
            for( int j = 0; j < nJ; ++j )
                for( int i = 0; i < nI; ++i ) {
                    int cellIndex = k*nJ*nI + j*nI + i;
                    InvisibiltyFlag currentFlag = (InvisibiltyFlag)
                            ( visibilityArray->GetValue( cellIndex ) );
                    if( i >= ui->sldILowClip->value() &&
                        i <= ui->sldIHighClip->value() &&
                        j >= ui->sldJLowClip->value() &&
                        j <= ui->sldJHighClip->value() &&
                        k >= ui->sldKLowClip->value() &&
                        k <=ui->sldKHighClip->value() ){
                        if( currentFlag == InvisibiltyFlag::INVISIBLE_NDV_AND_UVW_CLIPPING )
                            visibilityArray->SetValue( cellIndex, (int)InvisibiltyFlag::INVISIBLE_NDV_VALUE );
                        else if( currentFlag == InvisibiltyFlag::INVISIBLE_UVW_CLIPPING )
                            visibilityArray->SetValue( cellIndex, (int)InvisibiltyFlag::VISIBLE );
                    } else {
                        if( currentFlag == InvisibiltyFlag::VISIBLE )
                            visibilityArray->SetValue( cellIndex, (int)InvisibiltyFlag::INVISIBLE_UVW_CLIPPING );
                        else if( currentFlag == InvisibiltyFlag::INVISIBLE_NDV_VALUE )
                            visibilityArray->SetValue( cellIndex, (int)InvisibiltyFlag::INVISIBLE_NDV_AND_UVW_CLIPPING );
                    }

                }
        unstructuredGrid->GetCellData()->Modified();

    } else {
        Application::instance()->logError("V3DCfgWidForAttributeIn3DCartesianGrid::onUserMadeChanges(): null grid file.");
    }

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
