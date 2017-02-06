#include "widgetgslibparint.h"
#include "ui_widgetgslibparint.h"
#include "../gslibparint.h"

WidgetGSLibParInt::WidgetGSLibParInt(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetGSLibParInt)
{
    ui->setupUi(this);
}

WidgetGSLibParInt::~WidgetGSLibParInt()
{
    delete ui;
}

void WidgetGSLibParInt::fillFields(int param)
{
    ui->txtValue->setText( QString::number(param) );
}

void WidgetGSLibParInt::updateValue(uint *param)
{
    *param = ui->txtValue->text().toUInt();
}

void WidgetGSLibParInt::fillFields(GSLibParInt *param)
{
    ui->lblDesc->setText( param->getDescription() );
    ui->txtValue->setText( QString::number( param->_value ) );
}

void WidgetGSLibParInt::updateValue(GSLibParInt *param)
{
    param->_value = ui->txtValue->text().toInt();
}

void WidgetGSLibParInt::setLabelText(QString text)
{
    ui->lblDesc->setText( text );
}
