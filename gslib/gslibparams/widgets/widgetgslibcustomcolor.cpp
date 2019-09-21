#include "widgetgslibcustomcolor.h"
#include "ui_widgetgslibcustomcolor.h"
#include "gslib/gslibparams/gslibparcustomcolor.h"

#include <QColorDialog>

WidgetGSLibCustomColor::WidgetGSLibCustomColor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetGSLibCustomColor)
{
    ui->setupUi(this);
    ui->lblColorSample->setAutoFillBackground(true);
}

WidgetGSLibCustomColor::~WidgetGSLibCustomColor()
{
    delete ui;
}

void WidgetGSLibCustomColor::fillFields(GSLibParCustomColor *param)
{
    ui->spinR->setValue( param->_r );
    ui->spinG->setValue( param->_g );
    ui->spinB->setValue( param->_b );
    updateColorSample( 42 );
}

void WidgetGSLibCustomColor::updateValue(GSLibParCustomColor *param)
{
    param->_r = ui->spinR->value();
    param->_g = ui->spinG->value();
    param->_b = ui->spinB->value();
}

void WidgetGSLibCustomColor::updateColorSample(int value)
{
    Q_UNUSED( value );
    QString rgbaValues = QString::number(ui->spinR->value()) + "," +
                         QString::number(ui->spinG->value()) + "," +
                         QString::number(ui->spinB->value()) + ",255";

    ui->lblColorSample->setStyleSheet("QLabel { background-color : rgba(" + rgbaValues + "); }");
}

void WidgetGSLibCustomColor::openColorChooser()
{
    QColor color = QColorDialog::getColor( QColor( ui->spinR->value(), ui->spinG->value(), ui->spinB->value() ), this );
    if( color.isValid() ) {
        ui->spinR->setValue( color.red()   );
        ui->spinG->setValue( color.green() );
        ui->spinB->setValue( color.blue()  );
    }
}
