#include "pointsetdialog.h"
#include "ui_pointsetdialog.h"
#include <QFile>
#include <QTextStream>
#include "util.h"
#include "exceptions/invalidgslibdatafileexception.h"
#include <QMessageBox>


PointSetDialog::PointSetDialog(QWidget *parent, const QString file_path) :
    QDialog(parent),
    ui(new Ui::PointSetDialog)
{
    ui->setupUi(this);

    this->_file_path = file_path;

    //read file content sample
    Util::readFileSample( this->ui->txtFileSample, file_path );

    //get data file variable names to populate the combo boxes.
    try{
        QStringList field_list = Util::getFieldNames( file_path );
        ui->cmbX->addItem(" ");
        ui->cmbX->addItems( field_list );
        ui->cmbY->addItem(" ");
        ui->cmbY->addItems( field_list );
        ui->cmbZ->addItem(" ");
        ui->cmbZ->addItems( field_list );
    }catch ( InvalidGSLibDataFileException& ex ){
        QMessageBox::critical( this, "Error", "Invalid GSLib data file." );
    }

    adjustSize();
}

PointSetDialog::~PointSetDialog()
{
    delete ui;
}

int PointSetDialog::getXFieldIndex()
{
    return ui->cmbX->currentIndex();
}

int PointSetDialog::getYFieldIndex()
{
    return ui->cmbY->currentIndex();
}

int PointSetDialog::getZFieldIndex()
{
    return ui->cmbZ->currentIndex();
}

QString PointSetDialog::getNoDataValue()
{
    return ui->txtNDV->text();
}


