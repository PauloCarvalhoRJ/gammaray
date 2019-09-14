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

void AutomaticVarFitExperimentsDialog::setFromSpinBox(double min, double max, double increase)
{
    ui->dblSpinFrom->setMinimum( min );
    ui->dblSpinFrom->setMaximum( max );
    ui->dblSpinFrom->setSingleStep( increase );
}

void AutomaticVarFitExperimentsDialog::setToSpinBox(double min, double max, double increase)
{
    ui->dblSpinTo->setMinimum( min );
    ui->dblSpinTo->setMaximum( max );
    ui->dblSpinTo->setSingleStep( increase );
}

void AutomaticVarFitExperimentsDialog::setStepsSpinBox(int min, int max, int increase)
{
    ui->spinNumberOfSteps->setMinimum( min );
    ui->spinNumberOfSteps->setMaximum( max );
    ui->spinNumberOfSteps->setSingleStep( increase );
}

int AutomaticVarFitExperimentsDialog::getParameterIndex()
{
    return ui->cmbParameter->currentIndex();
}

double AutomaticVarFitExperimentsDialog::getFrom()
{
    return ui->dblSpinFrom->value();
}

double AutomaticVarFitExperimentsDialog::getTo()
{
    return ui->dblSpinTo->value();
}

int AutomaticVarFitExperimentsDialog::getNumberOfSteps()
{
    return ui->spinNumberOfSteps->value();
}
