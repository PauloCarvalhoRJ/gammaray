#include "widgetgslibpargrid.h"
#include "ui_widgetgslibpargrid.h"
#include "../../gslibparameterfiles/gslibparamtypes.h"
#include "widgetgslibparmultivaluedfixed.h"

WidgetGSLibParGrid::WidgetGSLibParGrid(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetGSLibParGrid)
{
    ui->setupUi(this);
}

WidgetGSLibParGrid::~WidgetGSLibParGrid()
{
    delete ui;
}

void WidgetGSLibParGrid::fillFields(GSLibParGrid *param)
{
    //remove all child widgets from ui->frmFields->layout() before adding
    //Util::clearChildWidgets( ui->frmFields );
    //remove all input widgets if any
    while( ! this->_widgets.empty() ){
        QWidget *w = this->_widgets.takeLast();
        ui->frmFields->layout()->removeWidget( w );
        /////////////////////  delete w;
    }

    ui->lblDesc->setText( "grid parameters" );

    //WidgetGSLibParMultiValuedFixed *new_widget = new WidgetGSLibParMultiValuedFixed( );
    WidgetGSLibParMultiValuedFixed *new_widget = (WidgetGSLibParMultiValuedFixed*)param->_specs_x->getWidget();
    new_widget->fillFields( param->_specs_x );
    ui->frmFields->layout()->addWidget( new_widget );
    this->_widgets.append( new_widget );

    //new_widget = new WidgetGSLibParMultiValuedFixed( );
    new_widget = (WidgetGSLibParMultiValuedFixed*)param->_specs_y->getWidget();
    new_widget->fillFields( param->_specs_y );
    ui->frmFields->layout()->addWidget( new_widget );
    this->_widgets.append( new_widget );

    //new_widget = new WidgetGSLibParMultiValuedFixed( );
    new_widget = (WidgetGSLibParMultiValuedFixed*)param->_specs_z->getWidget();
    new_widget->fillFields( param->_specs_z );
    ui->frmFields->layout()->addWidget( new_widget );
    this->_widgets.append( new_widget );
}

void WidgetGSLibParGrid::updateValue(GSLibParGrid *param)
{
    WidgetGSLibParMultiValuedFixed *mv_widget = (WidgetGSLibParMultiValuedFixed*)_widgets.at( 0 );
    mv_widget->updateValue( param->_specs_x );
    mv_widget = (WidgetGSLibParMultiValuedFixed*)_widgets.at( 1 );
    mv_widget->updateValue( param->_specs_y );
    mv_widget = (WidgetGSLibParMultiValuedFixed*)_widgets.at( 2 );
    mv_widget->updateValue( param->_specs_z );
}
