#include "svdparametersdialog.h"
#include "ui_svdparametersdialog.h"

SVDParametersDialog::SVDParametersDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SVDParametersDialog)
{
    ui->setupUi(this);
}

SVDParametersDialog::~SVDParametersDialog()
{
    delete ui;
}

long SVDParametersDialog::getNumberOfFactors()
{
    return ui->spinNumberOfFactors->value();
}
