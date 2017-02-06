#include "gslibparametersdialog.h"
#include "ui_gslibparametersdialog.h"
#include "gslibparameterfiles/gslibparameterfile.h"
#include "gslibparams/widgets/gslibparamwidgets.h"
#include "../domain/application.h"
#include "gslibparams/gslibparrepeat.h"

GSLibParametersDialog::GSLibParametersDialog(GSLibParameterFile *gpf, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GSLibParametersDialog),
    _gpf(gpf)
{
    ui->setupUi(this);

    //-----------------build the GUI according to the parameter object structure-----------------------
    this->setWindowTitle(QString("Parameters for ").append( _gpf->getProgramName().append(" program") ));
    QVBoxLayout *layout = new QVBoxLayout( ui->frmWidgets );
    layout->setSpacing(0);
    ui->frmWidgets->setLayout( layout );
    this->addParamWidgets();
    //-------------------end of GUI construction-------------------------------------------------------

    connect( this, SIGNAL( accepted() ), SLOT( onDialogAccepted() ) );
}

GSLibParametersDialog::~GSLibParametersDialog()
{
    //-----------------------------------------------------------------------------------
    //this code block detaches the parameter-owned widgets
    //from their Qt object parents, preventing their undue deletion
    //upon dialog destruction.  Such widgets can only be deleted when
    //the parent GSLibParType object is destroyed, othwerwise the program crashes
    //when this dialog is called a second time for the same GSLibParameterFile object .
    //A call to QLayout::addWidget(QWidget*) automatically sets QLayout as QWidget's parent.
    int cparams = this->_gpf->getParameterCount();
    for(int i = 0; i < cparams; ++i){
        GSLibParType* par = this->_gpf->getParameter<GSLibParType*>( i );
        par->detachFromGUI( ui->frmWidgets->layout() );
    }
    //-----------------------------------------------------------------------------------
    delete ui;
}

void GSLibParametersDialog::addParamWidgets()
{
    int cparams = this->_gpf->getParameterCount();
    //for each parameter in file
    for(int i = 0; i < cparams; ++i){
        //get pointer to parameter object
        GSLibParType* par = this->_gpf->getParameter<GSLibParType*>( i );
        //get the parameter's widget.
        QWidget* widget = par->getWidget();
        //if the parameter has a widget (may not have).
        if( widget ){
            //add the widget to the layout of this dialog
            ui->frmWidgets->layout()->addWidget( widget );
            //if the parameter is of <repeat> type, it referes to another parameter
            if( par->isRepeat() ){
                //I know the parameter is of repeat type
                GSLibParRepeat* rep_par = (GSLibParRepeat*)par;
                //get the name of the referenced parameter whose value is the repeat count
                QString referenced_parameter_name = rep_par->_ref_par_name;
                //try to find the referenced parameter
                GSLibParType* ref_par = this->_gpf->findParameterByName( referenced_parameter_name );
                if( ref_par ){
                    //if the parameter type has a signal assigned to value change events.
                    if( ref_par->getTypeName() == "uint" ){
                        // get the referenced parameter's widget
                        WidgetGSLibParUInt* ref_wiget = (WidgetGSLibParUInt*)ref_par->getWidget();
                        //connect its value change signal to one of the available slots of this dialog for this purpose.
                        connect( ref_wiget, SIGNAL(valueChanged(uint,QString)), this, SLOT(someUintWidgetValueChanged(uint,QString)) );
                    }else{
                        Application::instance()->logError( QString("GSLibParametersDialog::addParamWidgets(): referenced parameters of type \"").append( par->getTypeName() ).append("\" are currently not supported as referenced parameters.") );
                    }
                } else {
                    Application::instance()->logError( QString("GSLibParametersDialog::addParamWidgets(): referenced parameter [").append( referenced_parameter_name ).append("] not found in the file object's _parameters collection or within the inner collections (e.g. GSLibParMultivaluedFixed)") );
                }
            }
        }else
            Application::instance()->logError( QString("GSLibParametersDialog::addParamWidgets(): parameters of type \"").append( par->getTypeName() ).append("\" do not have a widget.") );
    }
}

void GSLibParametersDialog::onDialogAccepted()
{
    //update the GSLib parameter object with the user input values in the associated widgets.
    int cparams = this->_gpf->getParameterCount();
    for(int i = 0; i < cparams; ++i){
        GSLibParType* par = this->_gpf->getParameter<GSLibParType*>( i );
        if( ! par->update() ){
            Application::instance()->logError( QString("ERROR: parameters of type \"").append( par->getTypeName() ).append("\" are not editable or the parameter update failed.") );
        }
    }
}

void GSLibParametersDialog::someUintWidgetValueChanged(uint value, QString parameter_name)
{
    int cparams = this->_gpf->getParameterCount();
    //for each parameter in file
    for(int i = 0; i < cparams; ++i){
        //get pointer to parameter object
        GSLibParType* par = this->_gpf->getParameter<GSLibParType*>( i );
        //if the parameter is of <repeat> type, its count referes to another parameter value
        if( par->isRepeat() ){
            //I know the parameter is of repeat type
            GSLibParRepeat* rep_par = (GSLibParRepeat*)par;
            //get the name of the referenced parameter whose value is the repeat count
            QString referenced_parameter_name = rep_par->_ref_par_name;
            //if the referenced parameter name matches the parameter name whose value changed.
            if( parameter_name.compare( referenced_parameter_name ) == 0 ){
                //change the repeat parameter count
                rep_par->setCount( value );
                //GSLibParRepeat's getWidget causes an update in its widget.
                //TODO: however, any values entered prior to count change will be reset to the
                //      values stored in the contained GSLibPar* objects, so any values entered by the
                //      user will be lost if one changes the repeat count
                rep_par->getWidget();
            }
        }
    }
}

