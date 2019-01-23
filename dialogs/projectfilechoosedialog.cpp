#include "projectfilechoosedialog.h"
#include "ui_projectfilechoosedialog.h"

ProjectFileChooseDialog::ProjectFileChooseDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProjectFileChooseDialog)
{
    ui->setupUi(this);
}

ProjectFileChooseDialog::~ProjectFileChooseDialog()
{
    delete ui;
}
