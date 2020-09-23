#include "sectiondialog.h"
#include "ui_sectiondialog.h"

SectionDialog::SectionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SectionDialog)
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle( "Define a geologic section." );
}

SectionDialog::~SectionDialog()
{
    delete ui;
}
