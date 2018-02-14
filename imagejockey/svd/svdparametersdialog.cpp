#include "svdparametersdialog.h"
#include "ui_svdparametersdialog.h"

SVDParametersDialog::SVDParametersDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SVDParametersDialog)
{
    ui->setupUi(this);

    setWindowTitle( "Parameters for the SVD algorithm" );
}

SVDParametersDialog::~SVDParametersDialog()
{
    delete ui;
}

long SVDParametersDialog::getNumberOfFactors()
{
    return ui->spinNumberOfFactors->value();
}
