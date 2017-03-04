#include "widgetgslibparrepeat.h"
#include "ui_widgetgslibparrepeat.h"
#include "../../gslibparameterfiles/gslibparamtypes.h"
#include "widgetgslibparmultivaluedfixed.h"
#include "widgetgslibparmultivaluedvariable.h"
#include "widgetgslibparfile.h"
#include "widgetgslibparvmodel.h"
#include <QPainter>

WidgetGSLibParRepeat::WidgetGSLibParRepeat(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetGSLibParRepeat)
{
    ui->setupUi(this);
    this->setStyleSheet( ".WidgetGSLibParRepeat { \
                         border-color: rgb(0, 0, 0);\
                         border-style: solid;\
                         border-width: 1px;\
                     }" );
}

WidgetGSLibParRepeat::~WidgetGSLibParRepeat()
{
    delete ui;
}

void WidgetGSLibParRepeat::fillFields(GSLibParRepeat *param)
{
    //remove all child widgets from ui->frmFields->layout() before adding
    //Util::clearChildWidgets( ui->frmFields );
    //remove all input widgets if any
    while( ! this->_widgets.empty() ){
        QWidget *w = this->_widgets.takeLast();
        ui->verticalLayout->removeWidget( w );
        /////////// delete w; //do not delete widgets created within GSLibPar* objects
    }

    add_widgets( param->_original_parameters );
    add_widgets( param->_repeated_parameters );
}

void WidgetGSLibParRepeat::updateValue(GSLibParRepeat *param)
{
    //initialize a common widget iterator, since there is a single
    //widget list for both parameter lists
    QListIterator<QWidget *> it_wid( this->_widgets );
    update_values( param->_original_parameters, it_wid );
    update_values( param->_repeated_parameters, it_wid );
}

void WidgetGSLibParRepeat::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void WidgetGSLibParRepeat::add_widgets(QList<GSLibParType *> params_list)
{
    QListIterator<GSLibParType*> it( params_list );
    while (it.hasNext()){
        GSLibParType* par = it.next();
        QString param_type_name = par->getTypeName();
        if( param_type_name == "multivaluedfixed" ){
            //WidgetGSLibParMultiValuedFixed* new_widget = new WidgetGSLibParMultiValuedFixed( );
            WidgetGSLibParMultiValuedFixed* new_widget = (WidgetGSLibParMultiValuedFixed*)par->getWidget();
            new_widget->fillFields( (GSLibParMultiValuedFixed*)par );
            ui->verticalLayout->addWidget( new_widget );
            this->_widgets.append( new_widget );
        }else if( param_type_name == "file" ){
            //WidgetGSLibParFile* new_widget = new WidgetGSLibParFile( );
            WidgetGSLibParFile* new_widget = (WidgetGSLibParFile*)par->getWidget();
            new_widget->fillFields( (GSLibParFile*)par );
            ui->verticalLayout->addWidget( new_widget );
            this->_widgets.append( new_widget );
        }else if( param_type_name == "multivaluedvariable" ){
            WidgetGSLibParMultiValuedVariable* new_widget = (WidgetGSLibParMultiValuedVariable*)par->getWidget();
            new_widget->fillFields( (GSLibParMultiValuedVariable*)par );
            ui->verticalLayout->addWidget( new_widget );
            this->_widgets.append( new_widget );
        }else if( param_type_name == "vmodel" ){
            WidgetGSLibParVModel* new_widget = (WidgetGSLibParVModel*)par->getWidget();
            new_widget->fillFields( (GSLibParVModel*)par );
            ui->verticalLayout->addWidget( new_widget );
            this->_widgets.append( new_widget );
        }else
            Application::instance()->logError(QString("WidgetGSLibParRepeat::add_widgets(): parameter type \"").append(param_type_name).append("\" not fully supported as an array parameter."));
    }
}

void WidgetGSLibParRepeat::update_values(QList<GSLibParType *> params_list, QListIterator<QWidget *> &widget_iterator)
{
    QListIterator<GSLibParType*> it( params_list );
    while (it.hasNext()){
        GSLibParType* par = it.next();
        QWidget *widget = widget_iterator.next(); //also for each corresponding input widget
        QString param_type_name = par->getTypeName();
        if( param_type_name == "multivaluedfixed" ){
            WidgetGSLibParMultiValuedFixed* gwidget = (WidgetGSLibParMultiValuedFixed*)widget;
            gwidget->updateValue( (GSLibParMultiValuedFixed*)par );
        }else if( param_type_name == "file" ){
            WidgetGSLibParFile* gwidget = (WidgetGSLibParFile*)widget;
            gwidget->updateValue( (GSLibParFile*)par );
        }else if( param_type_name == "vmodel" ){
            WidgetGSLibParVModel* gwidget = (WidgetGSLibParVModel*)widget;
            gwidget->updateValue( (GSLibParVModel*)par );
        }else
            Application::instance()->logError(QString("WidgetGSLibParRepeat::update_values(): parameter type \"").append(param_type_name).append("\"  not fully supported as an array parameter."));
    }
}
