#include "gaborfilterdialog.h"
#include "ui_gaborfilterdialog.h"

GaborFilterDialog::GaborFilterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GaborFilterDialog)
{
    ui->setupUi(this);
}

GaborFilterDialog::~GaborFilterDialog()
{
    delete ui;
}
