#include "barchartwidget.h"
#include "ui_barchartwidget.h"

BarChartWidget::BarChartWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BarChartWidget)
{
    ui->setupUi(this);
}

BarChartWidget::~BarChartWidget()
{
    delete ui;
}
