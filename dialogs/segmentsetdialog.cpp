#include "segmentsetdialog.h"
#include "ui_segmentsetdialog.h"

SegmentSetDialog::SegmentSetDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SegmentSetDialog)
{
    ui->setupUi(this);
}

SegmentSetDialog::~SegmentSetDialog()
{
    delete ui;
}
