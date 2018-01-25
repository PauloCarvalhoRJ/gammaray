#include "svdfactorsselectiondialog.h"
#include "ui_svdfactorsselectiondialog.h"

SVDFactorsSelectionDialog::SVDFactorsSelectionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SVDFactorsSelectionDialog)
{
    ui->setupUi(this);

    setWindowTitle( "SVD factors selection" );
}

SVDFactorsSelectionDialog::~SVDFactorsSelectionDialog()
{
    delete ui;
}
