#include "emptydialog.h"
#include "ui_emptydialog.h"

#include <QLayout>

/** An empty dialog with vertical layout for dynamically-built dialogs. */
EmptyDialog::EmptyDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EmptyDialog)
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);
}

EmptyDialog::~EmptyDialog()
{
    delete ui;
}

void EmptyDialog::addWidget(QWidget *widget)
{
    this->layout()->addWidget( widget );
}
