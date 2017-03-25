#include "widgetgslibpardouble.h"
#include "ui_widgetgslibpardouble.h"
#include "../gslibpardouble.h"

WidgetGSLibParDouble::WidgetGSLibParDouble(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetGSLibParDouble)
{
    ui->setupUi(this);
}

WidgetGSLibParDouble::~WidgetGSLibParDouble()
{
    delete ui;
}

void WidgetGSLibParDouble::fillFields(double param, QString description)
{
    ui->lblDesc->setText( description );
    ui->lblDesc->setVisible( !description.isEmpty() );
    ui->txtValue->setText( QString::number( param, 'g', 12 ) );
}

void WidgetGSLibParDouble::updateValue(double *var)
{
    *var = ui->txtValue->text().toDouble();
}

void WidgetGSLibParDouble::fillFields(GSLibParDouble *param)
{
    fillFields( param->_value, param->getDescription() );
}

void WidgetGSLibParDouble::updateValue(GSLibParDouble *param)
{
    param->_value = ui->txtValue->text().toDouble();
}
