#include "variableselector.h"
#include "ui_variableselector.h"
#include "../domain/application.h"
#include "../domain/datafile.h"
#include "domain/attribute.h"

VariableSelector::VariableSelector(bool show_not_set, VariableSelectorType selectorType, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::VariableSelector),
    m_hasNotSetItem( show_not_set ),
    m_dataFile( nullptr ),
    m_selectorType( selectorType )
{
    ui->setupUi(this);

    //Forwarding the currentIndexChanged signal.
    connect( ui->cmbVariable, SIGNAL(currentIndexChanged(int)),
             this, SIGNAL(currentIndexChanged(int)));
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

void VariableSelector::addVariable(Attribute *at)
{
    if( ! m_dataFile )
        m_dataFile = dynamic_cast<DataFile*>( at->getContainingFile() );
    else{
        if( m_dataFile != dynamic_cast<DataFile*>(at->getContainingFile()) ){
            Application::instance()->logError("VariableSelector::addVariable(): attempt to add variables from different files.");
            return;
        }
    }
    ui->cmbVariable->addItem( at->getIcon(), at->getName() );
}

void VariableSelector::clear()
{
    ui->cmbVariable->clear();
}

Attribute *VariableSelector::getSelectedVariable()
{
    if( ! m_dataFile ){
        Application::instance()->logWarn("VariableSelector::getVariable(): m_dataFile == nullptr. Returning nullptr.");
        return nullptr;
    }
    //by selecting a variable name, surely the object is an Attribute
    Attribute* at = (Attribute*)m_dataFile->getChildByName( ui->cmbVariable->currentText() );
    if( ! at ){
        Application::instance()->logWarn("VariableSelector::getVariable(): Selection resulted in null attribute. Returning nullptr.");
        return nullptr;
    }
    return at;
}

int VariableSelector::getCurrentComboIndex()
{
    return ui->cmbVariable->currentIndex();
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
        ProjectComponent* pc = *it;
        if( pc->isAttribute() ){
            Attribute* attributeAspect = static_cast< Attribute* >( pc );
            bool canBeAdded =  m_selectorType == VariableSelectorType::ALL ||
                              (m_selectorType == VariableSelectorType::CATEGORICAL && attributeAspect->isCategorical());
            if( canBeAdded )
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
