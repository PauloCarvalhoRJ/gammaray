#include "widgetgslibcustomcolor.h"
#include "ui_widgetgslibcustomcolor.h"

WidgetGSLibCustomColor::WidgetGSLibCustomColor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WidgetGSLibCustomColor)
{
    ui->setupUi(this);
}

WidgetGSLibCustomColor::~WidgetGSLibCustomColor()
{
    delete ui;
}
