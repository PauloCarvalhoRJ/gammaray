#include "automaticvarfitexperimentsdialog.h"
#include "ui_automaticvarfitexperimentsdialog.h"

AutomaticVarFitExperimentsDialog::AutomaticVarFitExperimentsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AutomaticVarFitExperimentsDialog)
{
    ui->setupUi(this);
}

AutomaticVarFitExperimentsDialog::~AutomaticVarFitExperimentsDialog()
{
    delete ui;
}

void AutomaticVarFitExperimentsDialog::setAgorithmLabel(QString label)
{
    ui->lblOptimizationMethod->setText( label );
}

void AutomaticVarFitExperimentsDialog::setParameterList(QStringList names)
{
    ui->cmbParameter->addItems( names );
}

void AutomaticVarFitExperimentsDialog::setFromSpinBoxConfigs(
        const std::vector<MinMaxIncreaseDouble> oneConfigPerParameter)
{
    m_configsForFromSpinBox = oneConfigPerParameter;
}

void AutomaticVarFitExperimentsDialog::setToSpinBoxConfigs(
        const std::vector<MinMaxIncreaseDouble> oneConfigPerParameter)
{
    m_configsForToSpinBox = oneConfigPerParameter;
}

void AutomaticVarFitExperimentsDialog::setStepsSpinBoxConfigs(
        const std::vector<MinMaxIncreaseInt> oneConfigPerParameter)
{
    m_configsForStepsSpinBox = oneConfigPerParameter;
}

int AutomaticVarFitExperimentsDialog::getParameterIndex() const
{
    return ui->cmbParameter->currentIndex();
}

double AutomaticVarFitExperimentsDialog::getFrom() const
{
    return ui->dblSpinFrom->value();
}

double AutomaticVarFitExperimentsDialog::getTo() const
{
    return ui->dblSpinTo->value();
}

int AutomaticVarFitExperimentsDialog::getNumberOfSteps() const
{
    return ui->spinNumberOfSteps->value();
}

void AutomaticVarFitExperimentsDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent( event );
    onUpdateSpinBoxes();
}

void AutomaticVarFitExperimentsDialog::onUpdateSpinBoxes()
{
    int parameterIndex = ui->cmbParameter->currentIndex();
    if( parameterIndex < m_configsForFromSpinBox.size() ){
        ui->dblSpinFrom->setMinimum(    std::get<0>( m_configsForFromSpinBox[ parameterIndex ] ) );
        ui->dblSpinFrom->setMaximum(    std::get<1>( m_configsForFromSpinBox[ parameterIndex ] ) );
        ui->dblSpinFrom->setSingleStep( std::get<2>( m_configsForFromSpinBox[ parameterIndex ] ) );
        ui->dblSpinTo->setMinimum(    std::get<0>( m_configsForToSpinBox[ parameterIndex ] ) );
        ui->dblSpinTo->setMaximum(    std::get<1>( m_configsForToSpinBox[ parameterIndex ] ) );
        ui->dblSpinTo->setSingleStep( std::get<2>( m_configsForToSpinBox[ parameterIndex ] ) );
        ui->spinNumberOfSteps->setMinimum(    std::get<0>( m_configsForStepsSpinBox[ parameterIndex ] ) );
        ui->spinNumberOfSteps->setMaximum(    std::get<1>( m_configsForStepsSpinBox[ parameterIndex ] ) );
        ui->spinNumberOfSteps->setSingleStep( std::get<2>( m_configsForStepsSpinBox[ parameterIndex ] ) );
    }
}
