#include "widgetgslibparmultivaluedfixed.h"
#include "ui_widgetgslibparmultivaluedfixed.h"
#include "gslibparamwidgets.h"
#include "../../../domain/application.h"
#include "../gslibparmultivaluedfixed.h"
#include <QListIterator>
#include "../../../gslib/gslibparameterfiles/gslibparamtypes.h"
#include "util.h"

WidgetGSLibParMultiValuedFixed::WidgetGSLibParMultiValuedFixed(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetGSLibParMultiValuedFixed)
{
    ui->setupUi(this);

}

WidgetGSLibParMultiValuedFixed::~WidgetGSLibParMultiValuedFixed()
{
    delete ui;
}

void WidgetGSLibParMultiValuedFixed::fillFields( GSLibParMultiValuedFixed *param )
{
    //remove all child widgets from ui->frmFields->layout() before adding
    //Util::clearChildWidgets( ui->frmFields );
    //remove all input widgets if any
    while( ! this->_widgets.empty() ){
        QWidget *w = this->_widgets.takeLast();
        ui->frmFields->layout()->removeWidget( w );
        ////////////////////////////         delete w;  //do not delete widgets created within GSLibPar* objects
    }

    ui->lblDesc->setText( param->getDescription() );
    QListIterator<GSLibParType*> it( param->_parameters );
    while (it.hasNext()){
        GSLibParType* par = it.next();
        QString param_type_name = par->getTypeName();
        if( param_type_name == "int" ){
            //WidgetGSLibParInt* new_widget = new WidgetGSLibParInt( );
            WidgetGSLibParInt* new_widget = (WidgetGSLibParInt*)par->getWidget();
            new_widget->fillFields( ((GSLibParInt*)par)->_value );
            new_widget->setLabelText("");
            ui->frmFields->layout()->addWidget( new_widget );
            this->_widgets.append( new_widget );
        }else if( param_type_name == "double" ){
            //WidgetGSLibParDouble* new_widget = new WidgetGSLibParDouble( );
            WidgetGSLibParDouble* new_widget = (WidgetGSLibParDouble*)par->getWidget();
            new_widget->fillFields( ((GSLibParDouble*)par)->_value, "" );
            ui->frmFields->layout()->addWidget( new_widget );
            this->_widgets.append( new_widget );
        }else if( param_type_name == "uint" ){
            //WidgetGSLibParUInt* new_widget = new WidgetGSLibParUInt( );
            WidgetGSLibParUInt* new_widget = (WidgetGSLibParUInt*)par->getWidget();
            new_widget->fillFields( ((GSLibParUInt*)par)->_value, "" );
            ui->frmFields->layout()->addWidget( new_widget );
            this->_widgets.append( new_widget );
        }else if( param_type_name == "option" ){
            //WidgetGSLibParOption* new_widget = new WidgetGSLibParOption( );
            WidgetGSLibParOption* new_widget = (WidgetGSLibParOption*)par->getWidget();
            new_widget->fillFields( (GSLibParOption*)par, "" );
            ui->frmFields->layout()->addWidget( new_widget );
            this->_widgets.append( new_widget );
        }else if( param_type_name == "string" ){
            //WidgetGSLibParString* new_widget = new WidgetGSLibParString( );
            WidgetGSLibParString* new_widget = (WidgetGSLibParString*)par->getWidget();
            new_widget->fillFields( (GSLibParString*)par, "");
            ui->frmFields->layout()->addWidget( new_widget );
            this->_widgets.append( new_widget );
        }else if( param_type_name == "color" ){
            //WidgetGSLibParColor* new_widget = new WidgetGSLibParColor( );
            WidgetGSLibParColor* new_widget = (WidgetGSLibParColor*)par->getWidget();
            new_widget->fillFields( (GSLibParColor*)par, "");
            ui->frmFields->layout()->addWidget( new_widget );
            this->_widgets.append( new_widget );
        }else if( param_type_name == "multivaluedvariable" ){
            //WidgetGSLibParMultiValuedVariable* new_widget = new WidgetGSLibParMultiValuedVariable( ((GSLibParMultiValuedVariable*)par)->getAllowedParameterTypeName() );
            WidgetGSLibParMultiValuedVariable* new_widget = (WidgetGSLibParMultiValuedVariable*)par->getWidget();
            new_widget->fillFields( (GSLibParMultiValuedVariable*)par );
            ui->frmFields->layout()->addWidget( new_widget );
            this->_widgets.append( new_widget );
        }else
            Application::instance()->logError(QString("WidgetGSLibParMultiValuedFixed::fillFields(): parameter type \"").append(param_type_name).append("\" does not have an equivalent widget type."));
    }
}

void WidgetGSLibParMultiValuedFixed::updateValue(GSLibParMultiValuedFixed *param)
{
    QListIterator<GSLibParType *> it_par( param->_parameters );
    QListIterator<QWidget *> it_wid( this->_widgets );
    while (it_par.hasNext()){ //for each parameter in the object
        QWidget *widget = it_wid.next(); //also for each corresponding input widget
        GSLibParType* par = it_par.next();
        QString param_type_name = par->getTypeName();
        if( param_type_name == "int" ){
            WidgetGSLibParInt* gwidget = (WidgetGSLibParInt*)widget;
            gwidget->updateValue( (GSLibParInt*)par );
        }else if( param_type_name == "double" ){
            WidgetGSLibParDouble* gwidget = (WidgetGSLibParDouble*)widget;
            gwidget->updateValue( (GSLibParDouble*)par );
        }else if( param_type_name == "uint" ){
            WidgetGSLibParUInt* gwidget = (WidgetGSLibParUInt*)widget;
            gwidget->updateValue( (GSLibParUInt*)par );
        }else if( param_type_name == "option" ){
            WidgetGSLibParOption* gwidget = (WidgetGSLibParOption*)widget;
            gwidget->updateValue( (GSLibParOption*)par );
        }else if( param_type_name == "string" ){
            WidgetGSLibParString* gwidget = (WidgetGSLibParString*)widget;
            gwidget->updateValue( (GSLibParString*)par );
        }else if( param_type_name == "color" ){
            WidgetGSLibParColor* gwidget = (WidgetGSLibParColor*)widget;
            gwidget->updateValue( (GSLibParColor*)par );
        }else if( param_type_name == "multivaluedvariable" ){
            WidgetGSLibParMultiValuedVariable* gwidget = (WidgetGSLibParMultiValuedVariable*)widget;
            gwidget->updateValue( (GSLibParMultiValuedVariable*)par );
        }else
            Application::instance()->logError(QString("WidgetGSLibParMultiValuedFixed::updateValue(): parameter type \"").append(param_type_name).append("\" does not have an equivalent widget type."));
    }
}
