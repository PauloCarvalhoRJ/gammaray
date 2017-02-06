#include "widgetgslibparlimitsdouble.h"
#include "ui_widgetgslibparlimitsdouble.h"
#include "widgetgslibpardouble.h"
#include "../gslibparlimitsdouble.h"

WidgetGSLibParLimitsDouble::WidgetGSLibParLimitsDouble(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetGSLibParLimitsDouble)
{
    ui->setupUi(this);

    ui->frmFields->layout()->addWidget( _widgetMin = new WidgetGSLibParDouble( this ) );
    ui->frmFields->layout()->addWidget( _widgetMax = new WidgetGSLibParDouble( this ) );
}

WidgetGSLibParLimitsDouble::~WidgetGSLibParLimitsDouble()
{
    delete ui;
}

void WidgetGSLibParLimitsDouble::fillFields(GSLibParLimitsDouble *param)
{
    ui->lblDesc->setText( param->getDescription() );
    _widgetMin->fillFields( param->_min, "" );
    _widgetMax->fillFields( param->_max, "" );
}

void WidgetGSLibParLimitsDouble::updateValue(GSLibParLimitsDouble *param)
{
    _widgetMin->updateValue( &(param->_min) );
    _widgetMax->updateValue( &(param->_max) );
}
