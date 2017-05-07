#include "datafiledialog.h"
#include "ui_datafiledialog.h"
#include <QTextStream>
#include <QFile>
#include "util.h"

DataFileDialog::DataFileDialog(QWidget *parent, const QString file_path) :
    QDialog(parent),
    ui(new Ui::DataFileDialog)
{
    ui->setupUi(this);

    //default selected data file is undefined.
    this->_file_type = DataFileDialog::UNDEFINED;

    //read file content sample
    Util::readFileSample( this->ui->txtFileContent, file_path );

    adjustSize();
}

DataFileDialog::~DataFileDialog()
{
    delete ui;
}

int DataFileDialog::getDataFileType()
{
    return this->_file_type;
}

void DataFileDialog::accept()
{
    if( this->ui->radioGrid->isChecked() )
        this->_file_type = DataFileDialog::CARTESIANGRID;
    if ( this->ui->radioXYZ->isChecked() )
        this->_file_type = DataFileDialog::POINTSET;
    //close the dialog
    this->done( QDialog::Accepted );
}

