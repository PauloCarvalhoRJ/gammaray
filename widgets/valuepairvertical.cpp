#include "valuepairvertical.h"
#include "ui_valuepairvertical.h"

ValuePairVertical::ValuePairVertical(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ValuePairVertical)
{
    ui->setupUi(this);
}

ValuePairVertical::~ValuePairVertical()
{
    delete ui;
}

QString ValuePairVertical::get1st()
{
    return ui->txt1stValue->text();
}

QString ValuePairVertical::get2nd()
{
    return ui->txt2ndValue->text();
}

void ValuePairVertical::set1st(QString value)
{
    ui->txt1stValue->setText( value );
}

void ValuePairVertical::set2nd(QString value)
{
    ui->txt2ndValue->setText( value );
}
