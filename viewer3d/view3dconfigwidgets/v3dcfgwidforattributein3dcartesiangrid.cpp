#include "v3dcfgwidforattributein3dcartesiangrid.h"
#include "ui_v3dcfgwidforattributein3dcartesiangrid.h"

V3DCfgWidForAttributeIn3DCartesianGrid::V3DCfgWidForAttributeIn3DCartesianGrid(QWidget *parent) :
    View3DConfigWidget(parent),
    ui(new Ui::V3DCfgWidForAttributeIn3DCartesianGrid)
{
    ui->setupUi(this);
}

V3DCfgWidForAttributeIn3DCartesianGrid::~V3DCfgWidForAttributeIn3DCartesianGrid()
{
    delete ui;
}
