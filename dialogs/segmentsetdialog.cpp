#include "segmentsetdialog.h"
#include "ui_segmentsetdialog.h"
#include "exceptions/invalidgslibdatafileexception.h"
#include "util.h"
#include <QMessageBox>


SegmentSetDialog::SegmentSetDialog(QWidget *parent, const QString file_path) :
    QDialog(parent),
    ui(new Ui::SegmentSetDialog)
{
    ui->setupUi(this);

    setWindowTitle( "Segment set metadata" );

    this->_file_path = file_path;

    //read file content sample
    Util::readFileSample( this->ui->txtFileSample, file_path );

    //get data file variable names to populate the combo boxes.
    try{
        QStringList field_list = Util::getFieldNames( file_path );
        ui->cmbX0->addItem(" ");
        ui->cmbX0->addItems( field_list );
        ui->cmbY0->addItem(" ");
        ui->cmbY0->addItems( field_list );
        ui->cmbZ0->addItem(" ");
        ui->cmbZ0->addItems( field_list );
        ui->cmbX1->addItem(" ");
        ui->cmbX1->addItems( field_list );
        ui->cmbY1->addItem(" ");
        ui->cmbY1->addItems( field_list );
        ui->cmbZ1->addItem(" ");
        ui->cmbZ1->addItems( field_list );
    }catch ( InvalidGSLibDataFileException& ex ){
        QMessageBox::critical( this, "Error", "Invalid GSLib data file." );
    }

    adjustSize();
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

