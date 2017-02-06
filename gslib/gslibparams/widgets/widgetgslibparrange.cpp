#include "widgetgslibparrange.h"
#include "ui_widgetgslibparrange.h"
#include "../gslibparrange.h"

WidgetGSLibParRange::WidgetGSLibParRange(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetGSLibParRange),
    resolution(10)
{
    ui->setupUi(this);
}

WidgetGSLibParRange::~WidgetGSLibParRange()
{
    delete ui;
}

void WidgetGSLibParRange::fillFields(GSLibParRange *param)
{
    ui->lblDesc->setText( param->getDescription() );
    ui->hsliderValue->setMinimum( param->_min * resolution );
    ui->hsliderValue->setMaximum( param->_max * resolution );
    ui->hsliderValue->setValue( param->_value * resolution );
}

void WidgetGSLibParRange::updateValue(GSLibParRange *param)
{
    param->_value = ui->hsliderValue->value() / (double)resolution;
}
