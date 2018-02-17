#include "calculatordialog.h"
#include "ui_calculatordialog.h"

CalculatorDialog::CalculatorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalculatorDialog)
{
    ui->setupUi(this);
}

CalculatorDialog::~CalculatorDialog()
{
    delete ui;
}
