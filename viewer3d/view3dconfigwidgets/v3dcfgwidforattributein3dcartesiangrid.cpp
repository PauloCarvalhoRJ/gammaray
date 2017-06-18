#include "v3dcfgwidforattributein3dcartesiangrid.h"
#include "ui_v3dcfgwidforattributein3dcartesiangrid.h"

#include "domain/cartesiangrid.h"

V3DCfgWidForAttributeIn3DCartesianGrid::V3DCfgWidForAttributeIn3DCartesianGrid(
        CartesianGrid *cartesianGrid, Attribute *attribute, QWidget *parent) :
    View3DConfigWidget(parent),
    ui(new Ui::V3DCfgWidForAttributeIn3DCartesianGrid)
{
    ui->setupUi(this);

    ui->sldILowClip->setMinimum( 0 );
    ui->sldILowClip->setMaximum( cartesianGrid->getNX() );
    ui->sldIHighClip->setMinimum( 0 );
    ui->sldIHighClip->setMaximum( cartesianGrid->getNX() );

    ui->sldJLowClip->setMinimum( 0 );
    ui->sldJLowClip->setMaximum( cartesianGrid->getNY() );
    ui->sldJHighClip->setMinimum( 0 );
    ui->sldJHighClip->setMaximum( cartesianGrid->getNY() );

    ui->sldKLowClip->setMinimum( 0 );
    ui->sldKLowClip->setMaximum( cartesianGrid->getNZ() );
    ui->sldKHighClip->setMinimum( 0 );
    ui->sldKHighClip->setMaximum( cartesianGrid->getNZ() );
}

V3DCfgWidForAttributeIn3DCartesianGrid::~V3DCfgWidForAttributeIn3DCartesianGrid()
{
    delete ui;
}
