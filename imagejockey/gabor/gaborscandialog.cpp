#include "gaborscandialog.h"
#include "ui_gaborscandialog.h"

GaborScanDialog::GaborScanDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GaborScanDialog)
{
    ui->setupUi(this);
}

GaborScanDialog::~GaborScanDialog()
{
    delete ui;
}
