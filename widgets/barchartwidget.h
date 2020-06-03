#ifndef BARCHARTWIDGET_H
#define BARCHARTWIDGET_H

#include <QWidget>

namespace Ui {
class BarChartWidget;
}

class BarChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BarChartWidget(QWidget *parent = nullptr);
    ~BarChartWidget();

private:
    Ui::BarChartWidget *ui;
};

#endif // BARCHARTWIDGET_H
