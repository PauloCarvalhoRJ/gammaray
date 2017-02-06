#include "widgetgslibparvmodel.h"
#include "ui_widgetgslibparvmodel.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "widgetgslibparmultivaluedfixed.h"
#include "widgetgslibparrepeat.h"

WidgetGSLibParVModel::WidgetGSLibParVModel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetGSLibParVModel)
{
    ui->setupUi(this);
}

WidgetGSLibParVModel::~WidgetGSLibParVModel()
{
    delete ui;
}

void WidgetGSLibParVModel::fillFields(GSLibParVModel *param)
{
    //remove all child widgets from ui->frmFields->layout() before adding
    //Util::clearChildWidgets( ui->frmFields );
    //remove all input widgets if any
    while( ! this->_widgets.empty() ){
        QWidget *w = this->_widgets.takeLast();
        ui->frmFields->layout()->removeWidget( w );
        /////////////////////  delete w;
    }

    ui->lblCaption->setText( "variogram model parameters" );

    WidgetGSLibParMultiValuedFixed *new_widget = (WidgetGSLibParMultiValuedFixed*)param->_nst_and_nugget->getWidget();
    new_widget->fillFields( param->_nst_and_nugget );
    ui->frmFields->layout()->addWidget( new_widget );
    this->_widgets.append( new_widget );

    WidgetGSLibParRepeat *new_widget_r = (WidgetGSLibParRepeat*)param->_variogram_structures->getWidget();
    new_widget_r->fillFields( param->_variogram_structures );
    ui->frmFields->layout()->addWidget( new_widget_r );
    this->_widgets.append( new_widget_r );
}

void WidgetGSLibParVModel::updateValue(GSLibParVModel *param)
{
    WidgetGSLibParMultiValuedFixed *mv_widget = (WidgetGSLibParMultiValuedFixed*)_widgets.at( 0 );
    mv_widget->updateValue( param->_nst_and_nugget );
    WidgetGSLibParRepeat *mv_widget_r = (WidgetGSLibParRepeat*)_widgets.at( 1 );
    mv_widget_r->updateValue( param->_variogram_structures );
}
