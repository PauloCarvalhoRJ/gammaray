#include "gridresampledialog.h"
#include "ui_gridresampledialog.h"

#include "domain/cartesiangrid.h"

GridResampleDialog::GridResampleDialog(CartesianGrid *cg, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GridResampleDialog),
    _cg(cg)
{

    ui->setupUi(this);

    setWindowTitle("Grid resampling");

    ui->lblGridName->setText("<html><head/><body><p><span style=\" font-weight:600; color:#0000ff;\">" +
                             _cg->getName() + "</span></p></body></html>");
}

GridResampleDialog::~GridResampleDialog()
{
    delete ui;
}

int GridResampleDialog::getIrate()
{
    return ui->spinIRate->value();
}

int GridResampleDialog::getJrate()
{
    return ui->spinJRate->value();
}

int GridResampleDialog::getKrate()
{
    return ui->spinKRate->value();
}
