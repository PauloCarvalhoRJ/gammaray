#include "ijvariableselector.h"
#include "ui_ijvariableselector.h"
#include "../domain/application.h"
#include "../domain/datafile.h"
#include "domain/attribute.h"

IJVariableSelector::IJVariableSelector(bool show_not_set, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IJVariableSelector),
    m_hasNotSetItem( show_not_set ),
    m_dataFile( nullptr )
{
    ui->setupUi(this);

    //Forwarding the currentIndexChanged signal.
    connect( ui->cmbVariable, SIGNAL(currentIndexChanged(int)),
             this, SIGNAL(currentIndexChanged(int)));
}

IJVariableSelector::~IJVariableSelector()
{
    delete ui;
}

uint IJVariableSelector::getSelectedVariableGEOEASIndex()
{
    if( ! m_dataFile )
        return 0;
    if( m_hasNotSetItem )
        if( ui->cmbVariable->currentIndex() == 0 )
            return 0;
    QString var_name = ui->cmbVariable->currentText();
    return m_dataFile->getFieldGEOEASIndex( var_name );
}

QString IJVariableSelector::getSelectedVariableName()
{
    return ui->cmbVariable->currentText();
}

void IJVariableSelector::addVariable(Attribute *at)
{
    if( ! m_dataFile )
        m_dataFile = (DataFile*)at->getContainingFile();
    else{
        if( m_dataFile != (DataFile*)at->getContainingFile() ){
            Application::instance()->logError("IJVariableSelector::addVariable(): attempt to add variables from different files.");
            return;
        }
    }
    ui->cmbVariable->addItem( at->getIcon(), at->getName() );
}

void IJVariableSelector::clear()
{
    ui->cmbVariable->clear();
}

Attribute *IJVariableSelector::getSelectedVariable()
{
    if( ! m_dataFile ){
        Application::instance()->logWarn("IJVariableSelector::getVariable(): m_dataFile == nullptr. Returning nullptr.");
        return nullptr;
    }
    //by selecting a variable name, surely the object is an Attribute
    Attribute* at = (Attribute*)m_dataFile->getChildByName( ui->cmbVariable->currentText() );
    if( ! at ){
        Application::instance()->logWarn("IJVariableSelector::getVariable(): Selection resulted in null attribute. Returning nullptr.");
        return nullptr;
    }
    return at;
}

int IJVariableSelector::getCurrentComboIndex()
{
    return ui->cmbVariable->currentIndex();
}

void IJVariableSelector::setCaption(QString caption)
{
    ui->lblCaption->setText( caption );
}

void IJVariableSelector::onListVariables(DataFile *file)
{
    m_dataFile = file;
    ui->cmbVariable->clear();
    if( m_hasNotSetItem )
        ui->cmbVariable->addItem( "NOT SET" );
    if( ! file )
        return;
    std::vector<ProjectComponent*> all_contained_objects;
    file->getAllObjects( all_contained_objects );
    std::vector<ProjectComponent*>::iterator it = all_contained_objects.begin();
    for(; it != all_contained_objects.end(); ++it){
		ProjectComponent* pc = (ProjectComponent*)(*it);
        if( pc->isAttribute() ){
			ui->cmbVariable->addItem( pc->getIcon(), pc->getName() );
        }
    }
}

void IJVariableSelector::onSelection(int /*index*/)
{
    if( ! m_dataFile ){
        Application::instance()->logWarn("IJVariableSelector::onSelection(): Attempt to call this slot with m_dataFile == nullptr. Ignoring.");
        return;
    }
    //by selecting a variable name, surely the object is an Attribute
    Attribute* at = (Attribute*)m_dataFile->getChildByName( ui->cmbVariable->currentText() );
    if( ! at ){
        Application::instance()->logWarn("IJVariableSelector::onSelection(): Selection event resulted in null attribute. Ignoring.");
        return;
    }
    emit variableSelected( at );
    return;
}
