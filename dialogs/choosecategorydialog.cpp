#include "choosecategorydialog.h"
#include "ui_choosecategorydialog.h"

ChooseCategoryDialog::ChooseCategoryDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChooseCategoryDialog)
{
    ui->setupUi(this);
}

ChooseCategoryDialog::~ChooseCategoryDialog()
{
    delete ui;
}
