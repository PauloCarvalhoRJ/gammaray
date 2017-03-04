#include "widgetgslibparmultivaluedvariable.h"
#include "ui_widgetgslibparmultivaluedvariable.h"
#include "gslibparamwidgets.h"
#include "../../../domain/application.h"
#include "../gslibparvarweight.h"
#include <QListIterator>
#include "../../../gslib/gslibparameterfiles/gslibparamtypes.h"

WidgetGSLibParMultiValuedVariable::WidgetGSLibParMultiValuedVariable(const QString gslib_parameter_type_name, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetGSLibParMultiValuedVariable)
{
    ui->setupUi(this);
    this->_gslib_parameter_type_name = gslib_parameter_type_name;
    connect(ui->btnAdd, SIGNAL(clicked(bool)), this, SLOT(onBtnAddClicked(bool)));
    connect(ui->btnRemove, SIGNAL(clicked(bool)), this, SLOT(onBtnRemoveClicked(bool)));
}

WidgetGSLibParMultiValuedVariable::~WidgetGSLibParMultiValuedVariable()
{
    delete ui;
}

void WidgetGSLibParMultiValuedVariable::fillFields(QList<GSLibParVarWeight *> *param)
{
    ui->lblDesc->setText( "Variable and weight indexes" );
    //remove all input widgets if any
    while( ! this->_widgets.empty() ){
        QWidget *w = this->_widgets.takeLast();
        ui->frmFields->layout()->removeWidget( w );
        delete w; //when the widgets are created for the QList, then this object manages them.
                  //since QList does not create the widgets
    }
    //add widgets according to the GSLib parameter
    if( this->_gslib_parameter_type_name == "int" ){
        QListIterator<GSLibParVarWeight *> it( *param );
        //first, the variables indexes...
        int count = 0;
        while( it.hasNext() ){
            GSLibParVarWeight *var_wgt = it.next();
            WidgetGSLibParInt *new_widget = new WidgetGSLibParInt();
            new_widget->setLabelText(QString("variable ").append(QString::number(++count)));
            this->_widgets.append( new_widget );
            ui->frmFields->layout()->addWidget( new_widget );
            new_widget->fillFields( var_wgt->_var_index );
        }
        //...then, the weight indexes.
        count = 0;
        it.toFront(); //go back to the first pair variable/wieght indexes
        while( it.hasNext() ){
            GSLibParVarWeight *var_wgt = it.next();
            WidgetGSLibParInt *new_widget = new WidgetGSLibParInt();
            new_widget->setLabelText(QString("weight ").append(QString::number(++count)));
            this->_widgets.append( new_widget );
            ui->frmFields->layout()->addWidget( new_widget );
            new_widget->fillFields( var_wgt->_wgt_index );
        }
    }else
        Application::instance()->logError(QString("WidgetGSLibParMultiValuedVariable::fillFields(QList<GSLibParVarWeight*>*): Parameter type \"").append(_gslib_parameter_type_name).append("\" not supported."));
}

void WidgetGSLibParMultiValuedVariable::updateValue(QList<GSLibParVarWeight *> *param)
{
    //read user-entered values from the widgets to update the GSLib parameters
    if( this->_gslib_parameter_type_name == "int" ){
        QListIterator<GSLibParVarWeight *> it_par( *param );
        QListIterator<QWidget *> it_wid( this->_widgets );
        //first, the variables indexes...
        while( it_wid.hasNext() ){
            GSLibParVarWeight *var_wgt = it_par.next();
            WidgetGSLibParInt *widget = (WidgetGSLibParInt *)it_wid.next();
            widget->updateValue( &(var_wgt->_var_index) );
            if( ! it_par.hasNext() ) //finished reading the variable indexes
                break; //abort the loop
        }
        //...then, the weight indexes.
        it_par.toFront(); //go back to the first pair variable/weight indexes
        while( it_wid.hasNext() ){ //continue reading the second half of the input widgets with the weight indexes
            GSLibParVarWeight *var_wgt = it_par.next();
            WidgetGSLibParInt *widget = (WidgetGSLibParInt *)it_wid.next();
            widget->updateValue( &(var_wgt->_wgt_index) );
        }
    }else
        Application::instance()->logError(QString("WidgetGSLibParMultiValuedVariable::updateValue(QList<GSLibParVarWeight*>*): Parameter type \"").append(_gslib_parameter_type_name).append("\" not supported."));
}

void WidgetGSLibParMultiValuedVariable::fillFields(GSLibParMultiValuedVariable *param)
{
    //remove all child widgets from ui->frmFields->layout() before adding
    //Util::clearChildWidgets( ui->frmFields );
    //remove all input widgets if any
    while( ! this->_widgets.empty() ){
        QWidget *w = this->_widgets.takeLast();
        ui->frmFields->layout()->removeWidget( w );
        //////////////////  delete w;
    }

    ui->lblDesc->setText( param->getDescription() );
    QListIterator<GSLibParType*> it( param->_parameters );
    while (it.hasNext()){
        GSLibParType* par = it.next();
        QString param_type_name = par->getTypeName();
        if( param_type_name == "uint" ){
            //WidgetGSLibParUInt* new_widget = new WidgetGSLibParUInt( );
            WidgetGSLibParUInt* new_widget = (WidgetGSLibParUInt*)par->getWidget();
            new_widget->fillFields( ((GSLibParUInt*)par)->_value, "" );
            ui->frmFields->layout()->addWidget( new_widget );
            this->_widgets.append( new_widget );
        } else if( param_type_name == "double" ){
            WidgetGSLibParDouble* new_widget = (WidgetGSLibParDouble*)par->getWidget();
            new_widget->fillFields( (GSLibParDouble*)par );
            ui->frmFields->layout()->addWidget( new_widget );
            this->_widgets.append( new_widget );
        }else
            Application::instance()->logError(QString("WidgetGSLibParMultiValuedVariable::fillFields(): parameter type \"").append(param_type_name).append("\" does not have an equivalent widget type."));
    }
}

void WidgetGSLibParMultiValuedVariable::updateValue(GSLibParMultiValuedVariable *param)
{
    //QListIterator<GSLibParType *> it_par( param->_parameters );
    QList<GSLibParType *>::iterator it_par = param->_parameters.begin();
    QListIterator<QWidget *> it_wid( this->_widgets );
    while (it_par != param->_parameters.end() && it_wid.hasNext() ){ //for each parameter in the object
        QWidget *widget = it_wid.next(); //also for each corresponding input widget
        GSLibParType* par = *it_par; ++it_par;
        QString param_type_name = par->getTypeName();
        if( param_type_name == "uint" ){
            WidgetGSLibParUInt* gwidget = (WidgetGSLibParUInt*)widget;
            gwidget->updateValue( (GSLibParUInt*)par );
        }else if( param_type_name == "double" ){
            WidgetGSLibParDouble* gwidget = (WidgetGSLibParDouble*)widget;
            gwidget->updateValue( (GSLibParDouble*)par );
        }else
            Application::instance()->logError(QString("WidgetGSLibParMultiValuedVariable::updateValue(GSLibParMultiValuedVariable*): parameter type \"").append(param_type_name).append("\" does not have an equivalent widget type."));
    }
    //maybe the user added widgets, so it is necessary
    //to create additional parameter value elements for each additional widget
    bool can_remove_parameters = true;
    while( it_wid.hasNext() ){
        can_remove_parameters = false; //since we're adding parameters, this prevents the parameter removal loop from triggering
        QWidget *widget = it_wid.next();
        if( _gslib_parameter_type_name == "uint" ){
            GSLibParUInt* par = new GSLibParUInt("","","");
            WidgetGSLibParUInt* gwidget = (WidgetGSLibParUInt*)widget;
            gwidget->updateValue( par );
            param->_parameters.append( par );
        }else
            Application::instance()->logError(QString("WidgetGSLibParMultiValuedVariable::updateValue(GSLibParMultiValuedVariable*) 2: parameter type \"").append(_gslib_parameter_type_name).append("\" does not have an equivalent widget type."));
    }
    //maybe the user removed widgets, so it is necessary
    //to delete the excess parameter value elements
    while( can_remove_parameters && it_par != param->_parameters.end() ){
        //FIXME: check whether it is necessary to de-allocate the object in the list.
        it_par = param->_parameters.erase( it_par );
    }
}

void WidgetGSLibParMultiValuedVariable::onBtnAddClicked(bool)
{
    if( this->_gslib_parameter_type_name == "int" ){
        this->_widgets.append( new WidgetGSLibParInt( ) );
        ui->frmFields->layout()->addWidget( this->_widgets.back() );
    }else if( this->_gslib_parameter_type_name == "uint" ){
        WidgetGSLibParUInt* widget = new WidgetGSLibParUInt( );
        widget->fillFields(0, "");
        this->_widgets.append( widget );
        ui->frmFields->layout()->addWidget( widget );
    }else
        Application::instance()->logError(QString("WidgetGSLibParMultiValuedVariable::onBtnAddClicked(): Parameter type \"").append(_gslib_parameter_type_name).append("\" does not have an equivalent widget type."));
}

void WidgetGSLibParMultiValuedVariable::onBtnRemoveClicked(bool)
{
    if( ! this->_widgets.empty() ){
        QWidget *w = this->_widgets.takeLast();
        ui->frmFields->layout()->removeWidget( w );
        delete w;
    }
}
