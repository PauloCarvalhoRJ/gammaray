#include "postikdialog.h"
#include "ui_postikdialog.h"
#include "domain/application.h"
#include "widgets/cartesiangridselector.h"
#include "widgets/fileselectorwidget.h"
#include "widgets/variableselector.h"

PostikDialog::PostikDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PostikDialog)
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle("Indicator Kriging post-processing");

    //The list with existing grids in the project.
    m_Ik3dGridSelector = new CartesianGridSelector();
    ui->frmIk3dOutput->layout()->addWidget( m_Ik3dGridSelector );

    //the list with the threshold c.d.f.'s containing the thresholds
    m_thresholdsSelector = new FileSelectorWidget( FileSelectorType::CDFs );
    ui->frmThresholds->layout()->addWidget( m_thresholdsSelector );

    //the list with data files to inform the distribution (optional)
    m_fileForDistSelector = new FileSelectorWidget( FileSelectorType::DataFiles, true );
    ui->frmDataForDistribution->layout()->addWidget( m_fileForDistSelector );

    //The list with the variables to inform the distribution
    m_variableForDistSelector = new VariableSelector();
    ui->frmDataForDistribution->layout()->addWidget( m_variableForDistSelector );
    connect( m_fileForDistSelector, SIGNAL(dataFileSelected(DataFile*)),
             m_variableForDistSelector, SLOT(onListVariables(DataFile*)) );

    //The list with the declustering weight to inform the distribution
    m_weightForDistSelector = new VariableSelector( true );
    ui->frmDataForDistribution->layout()->addWidget( m_weightForDistSelector );
    connect( m_fileForDistSelector, SIGNAL(dataFileSelected(DataFile*)),
             m_weightForDistSelector, SLOT(onListVariables(DataFile*)) );

    adjustSize();

}

PostikDialog::~PostikDialog()
{
    delete ui;
    Application::instance()->logInfo("PostikDialog destroyed.");
}

void PostikDialog::onConfigureAndRun()
{

}

void PostikDialog::onPostikCompletes()
{

}

void PostikDialog::onSave()
{

}
