#include "variableselector.h"
#include "ui_variableselector.h"
#include "../domain/application.h"
#include "../domain/datafile.h"

VariableSelector::VariableSelector(bool show_not_set, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VariableSelector),
    m_hasNotSetItem( show_not_set ),
    m_dataFile( nullptr )
{
    ui->setupUi(this);
}

VariableSelector::~VariableSelector()
{
    delete ui;
}

uint VariableSelector::getSelectedVariableGEOEASIndex()
{
    if( ! m_dataFile )
        return 0;
    if( m_hasNotSetItem )
        if( ui->cmbVariable->currentIndex() == 0 )
            return 0;
    QString var_name = ui->cmbVariable->currentText();
    return m_dataFile->getFieldGEOEASIndex( var_name );
}

QString VariableSelector::getSelectedVariableName()
{
    return ui->cmbVariable->currentText();
}

void VariableSelector::setCaption(QString caption)
{
    ui->lblCaption->setText( caption );
}

void VariableSelector::onListVariables(DataFile *file)
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

void VariableSelector::onSelection(int /*index*/)
{
    if( ! m_dataFile ){
        Application::instance()->logWarn("VariableSelector::onSelection(): Attempt to call this slot with m_dataFile == nullptr. Ignoring.");
        return;
    }
    //by selecting a variable name, surely the object is an Attribute
    Attribute* at = (Attribute*)m_dataFile->getChildByName( ui->cmbVariable->currentText() );
    if( ! at ){
        Application::instance()->logWarn("VariableSelector::onSelection(): Selection event resulted in null attribute. Ignoring.");
        return;
    }
    emit variableSelected( at );
    return;
}
