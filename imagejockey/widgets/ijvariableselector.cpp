#include "ijvariableselector.h"
#include "ui_ijvariableselector.h"
#include "../ijabstractcartesiangrid.h"
#include "../ijabstractvariable.h"

IJVariableSelector::IJVariableSelector(bool show_not_set, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IJVariableSelector),
    m_hasNotSetItem( show_not_set ),
    m_grid( nullptr )
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

uint IJVariableSelector::getSelectedVariableIndex()
{
    if( ! m_grid )
        return 0;
    if( m_hasNotSetItem )
        if( ui->cmbVariable->currentIndex() == 0 )
            return 0;
    QString var_name = ui->cmbVariable->currentText();
    return m_grid->getVariableIndexByName( var_name );
}

QString IJVariableSelector::getSelectedVariableName()
{
    return ui->cmbVariable->currentText();
}

void IJVariableSelector::addVariable(IJAbstractVariable *var)
{
    if( ! m_grid )
        m_grid = var->getParentGrid();
    else{
        if( m_grid != var->getParentGrid() ){
            emit errorOccurred("IJVariableSelector::addVariable(): attempt to add variables from different grids.");
            return;
        }
    }
    ui->cmbVariable->addItem( var->getVariableIcon(), var->getVariableName() );
}

void IJVariableSelector::clear()
{
    ui->cmbVariable->clear();
}

IJAbstractVariable *IJVariableSelector::getSelectedVariable()
{
    if( ! m_grid ){
        emit warningOccurred("IJVariableSelector::getSelectedVariable(): m_grid == nullptr. Returning nullptr.");
        return nullptr;
    }
    //by selecting a variable name, surely the object is an Attribute
    IJAbstractVariable* variable = m_grid->getVariableByName( ui->cmbVariable->currentText() );
    if( ! variable ){
        emit warningOccurred("IJVariableSelector::getSelectedVariable(): Selection resulted in null attribute. Returning nullptr.");
        return nullptr;
    }
    return variable;
}

int IJVariableSelector::getCurrentComboIndex()
{
    return ui->cmbVariable->currentIndex();
}

void IJVariableSelector::setCaption(QString caption)
{
    ui->lblCaption->setText( caption );
}

void IJVariableSelector::onListVariables(IJAbstractCartesianGrid *grid)
{
    m_grid = grid;
    ui->cmbVariable->clear();
    if( m_hasNotSetItem )
        ui->cmbVariable->addItem( "NOT SET" );
    if( ! grid )
        return;
    std::vector<IJAbstractVariable*> all_contained_variables;
    grid->getAllVariables( all_contained_variables );
    std::vector<IJAbstractVariable*>::iterator it = all_contained_variables.begin();
    for(; it != all_contained_variables.end(); ++it){
        IJAbstractVariable* var = *it;
        ui->cmbVariable->addItem( var->getVariableIcon(), var->getVariableName() );
    }
}

void IJVariableSelector::onSelection(int /*index*/)
{
    if( ! m_grid ){
        emit warningOccurred("IJVariableSelector::onSelection(): Attempt to call this slot with m_grid == nullptr. Ignoring.");
        return;
    }
    //by selecting a variable name, surely the object is an Attribute
    IJAbstractVariable* var = m_grid->getVariableByName( ui->cmbVariable->currentText() );
    if( ! var ){
        emit warningOccurred("IJVariableSelector::onSelection(): Selection event resulted in null attribute. Ignoring.");
        return;
    }
    emit variableSelected( var );
    return;
}
