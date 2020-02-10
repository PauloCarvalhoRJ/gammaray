#include "mcmcdataimputationdialog.h"
#include "ui_mcmcdataimputationdialog.h"

#include "widgets/fileselectorwidget.h"
#include "widgets/variableselector.h"


MCMCDataImputationDialog::MCMCDataImputationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MCMCDataImputationDialog)
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle( "Data imputation with Markov Chains-Monte Carlo simulation" );

    m_fileSelector = new FileSelectorWidget( FileSelectorType::SegmentSets );
    ui->frmCmbDataSet->layout()->addWidget( m_fileSelector );

    m_varSelector = new VariableSelector( false, VariableSelectorType::CATEGORICAL );
    ui->frmCmbVariable->layout()->addWidget( m_varSelector );
    connect( m_fileSelector, SIGNAL(dataFileSelected(DataFile*)),
             m_varSelector,  SLOT(onListVariables(DataFile*)) );

    //calling this slot causes the variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_fileSelector->onSelection( 0 );
}

MCMCDataImputationDialog::~MCMCDataImputationDialog()
{
    delete ui;
}
