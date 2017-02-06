#include "widgetgslibparuint.h"
#include "ui_widgetgslibparuint.h"
#include "../gslibparuint.h"
#include "../../../domain/application.h"

WidgetGSLibParUInt::WidgetGSLibParUInt(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetGSLibParUInt),
    _last_parameter_referenced( nullptr )
{
    ui->setupUi(this);
}

WidgetGSLibParUInt::~WidgetGSLibParUInt()
{
    delete ui;
}

void WidgetGSLibParUInt::fillFields(GSLibParUInt *param)
{
    ui->label->setText( param->getDescription() );
    ui->txtUInt->setText( QString::number( param->_value ) );
    _last_parameter_referenced = param;
}

void WidgetGSLibParUInt::fillFields(uint value, QString description)
{
    ui->label->setText( description );
    ui->label->setVisible( !description.isEmpty() );
    ui->txtUInt->setText( QString::number( value ) );
}

void WidgetGSLibParUInt::updateValue(GSLibParUInt *param)
{
    param->_value = ui->txtUInt->text().toUInt();
    _last_parameter_referenced = param;
}

void WidgetGSLibParUInt::textChanged(const QString &text)
{
    QString parameter_name = "";
    if( _last_parameter_referenced )
        parameter_name = _last_parameter_referenced->getName();
    emit valueChanged( text.toUInt(), parameter_name );
}
