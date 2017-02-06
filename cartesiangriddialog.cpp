#include "cartesiangriddialog.h"
#include "ui_cartesiangriddialog.h"
#include "util.h"
#include <QTextStream>


CartesianGridDialog::CartesianGridDialog(QWidget *parent, const QString file_path) :
    QDialog(parent),
    ui(new Ui::CartesianGridDialog)
{
    ui->setupUi(this);

    //read file content sample
    Util::readFileSample( this->ui->txtFileSample, file_path );
}

CartesianGridDialog::~CartesianGridDialog()
{
    delete ui;
}

double CartesianGridDialog::getX0()
{
    return ui->txtX0->text().toDouble();
}

double CartesianGridDialog::getY0()
{
    return ui->txtY0->text().toDouble();
}

double CartesianGridDialog::getZ0()
{
    return ui->txtZ0->text().toDouble();
}

double CartesianGridDialog::getDX()
{
    return ui->txtDX->text().toDouble();
}

double CartesianGridDialog::getDY()
{
    return ui->txtDY->text().toDouble();
}

double CartesianGridDialog::getDZ()
{
    return ui->txtDZ->text().toDouble();
}

uint CartesianGridDialog::getNX()
{
    return ui->txtNX->text().toUInt();
}

uint CartesianGridDialog::getNY()
{
    return ui->txtNY->text().toUInt();
}

uint CartesianGridDialog::getNZ()
{
    return ui->txtNZ->text().toUInt();
}

uint CartesianGridDialog::getNReal()
{
    return ui->txtRealizationCount->text().toUInt();
}

double CartesianGridDialog::getRot()
{
    return ui->txtRotation->text().toDouble();
}

QString CartesianGridDialog::getNoDataValue()
{
    return ui->txtNDV->text();
}
