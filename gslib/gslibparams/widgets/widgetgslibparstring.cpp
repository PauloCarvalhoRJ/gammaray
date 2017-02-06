#include "widgetgslibparstring.h"
#include "ui_widgetgslibparstring.h"
#include "../gslibparstring.h"


WidgetGSLibParString::WidgetGSLibParString(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetGSLibParString)
{
    ui->setupUi(this);
}

WidgetGSLibParString::~WidgetGSLibParString()
{
    delete ui;
}

void WidgetGSLibParString::fillFields(GSLibParString *param)
{
    this->fillFields( param, param->getDescription() );
}

void WidgetGSLibParString::fillFields(GSLibParString *param, const QString label)
{
    ui->lblDesc->setText( label );
    ui->txtValue->setText( param->_value );
}

void WidgetGSLibParString::updateValue(GSLibParString *param)
{
    param->_value = ui->txtValue->text();
}
