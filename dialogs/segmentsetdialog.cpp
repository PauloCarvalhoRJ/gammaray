#include "segmentsetdialog.h"
#include "ui_segmentsetdialog.h"

SegmentSetDialog::SegmentSetDialog(QWidget *parent, const QString file_path) :
    QDialog(parent),
    ui(new Ui::SegmentSetDialog)
{
    ui->setupUi(this);
}

SegmentSetDialog::~SegmentSetDialog()
{
    delete ui;
}

int SegmentSetDialog::getXIniFieldIndex()
{
    return ui->cmbX0->currentIndex();
}

int SegmentSetDialog::getYIniFieldIndex()
{
    return ui->cmbY0->currentIndex();
}

int SegmentSetDialog::getZIniFieldIndex()
{
    return ui->cmbZ0->currentIndex();
}

QString SegmentSetDialog::getNoDataValue()
{
    return ui->txtNDV->text();
}

int SegmentSetDialog::getXFinFieldIndex()
{
    return ui->cmbX1->currentIndex();
}

int SegmentSetDialog::getYFinFieldIndex()
{
    return ui->cmbY1->currentIndex();
}

int SegmentSetDialog::getZFinFieldIndex()
{
    return ui->cmbZ1->currentIndex();
}

