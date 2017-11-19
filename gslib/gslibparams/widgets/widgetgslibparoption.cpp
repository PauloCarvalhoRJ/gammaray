#include "widgetgslibparoption.h"
#include "ui_widgetgslibparoption.h"
#include "../gslibparoption.h"

WidgetGSLibParOption::WidgetGSLibParOption(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetGSLibParOption)
{
    ui->setupUi(this);
}

WidgetGSLibParOption::~WidgetGSLibParOption()
{
    delete ui;
}

void WidgetGSLibParOption::fillFields(GSLibParOption *param)
{
    //reset combobox
    ui->cmbValue->clear();
    //set label
    ui->lblDesc->setText( param->getDescription() );
    //populate combo box with the available options
    for( QMap<int, QString>::iterator it = param->_options.begin(); it != param->_options.end(); ++it){
        ui->cmbValue->addItem( it.value(), QVariant( it.key() ) );
        //sets the option corresponding to the current parameter value
        if( it.key() == param->_selected_value )
            ui->cmbValue->setCurrentIndex( ui->cmbValue->count() - 1 );
    }
}

void WidgetGSLibParOption::fillFields(GSLibParOption *param, QString description)
{
    fillFields( param );
    ui->lblDesc->setText( description );
    ui->lblDesc->setVisible( !description.isEmpty() );
}

void WidgetGSLibParOption::updateValue(GSLibParOption *param)
{
    param->_selected_value = ui->cmbValue->itemData( ui->cmbValue->currentIndex() ).toInt();
}
