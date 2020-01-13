#include "choosevariabledialog.h"
#include "ui_choosevariabledialog.h"

#include "widgets/variableselector.h"

ChooseVariableDialog::ChooseVariableDialog(DataFile *df, const QString title, const QString caption, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChooseVariableDialog),
    m_df(df)
{
    ui->setupUi(this);
    m_variableSelector = new VariableSelector( true );
    m_variableSelector->onListVariables( df );
    ui->layoutForCategorySelector->addWidget( m_variableSelector );
    ui->lblCaption->setText( caption );
    setWindowTitle( title );
}

ChooseVariableDialog::~ChooseVariableDialog()
{
    delete ui;
}

int ChooseVariableDialog::getSelectedVariableIndex()
{
    return static_cast<int>( m_variableSelector->getSelectedVariableGEOEASIndex() ) - 1;
}
