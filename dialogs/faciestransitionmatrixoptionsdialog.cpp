#include "faciestransitionmatrixoptionsdialog.h"
#include "ui_faciestransitionmatrixoptionsdialog.h"

FaciesTransitionMatrixOptionsDialog::FaciesTransitionMatrixOptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FaciesTransitionMatrixOptionsDialog)
{
    ui->setupUi(this);

    setWindowTitle("Options for FTM making.");
}

FaciesTransitionMatrixOptionsDialog::~FaciesTransitionMatrixOptionsDialog()
{
    delete ui;
}
