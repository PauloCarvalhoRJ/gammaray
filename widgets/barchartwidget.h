#ifndef BARCHARTWIDGET_H
#define BARCHARTWIDGET_H

#include <QWidget>

namespace Ui {
class BarChartWidget;
}

namespace QtCharts{
class QChart;
class QChartView;
class QBarSeries;
}

class BarChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BarChartWidget(QWidget *parent = nullptr);
    ~BarChartWidget();

    /** Set the title of the plot. */
    void setChartTitle( const QString title );

    /** Adds a bar set consisting of a single bar (common histogram). */
    void addBar(const QString categoryName, double value , const QColor color);

    /** Sets the maximum value for the Y axis of the chart. */
    void setMaxY( double value );

    void setAxisYLabel( const QString label );

    void setAxisXLabel( const QString label );

private:
    Ui::BarChartWidget *ui;

    QtCharts::QChart* m_chart;
    QtCharts::QChartView* m_chartView;
    QtCharts::QBarSeries* m_series;
};

#endif // BARCHARTWIDGET_H
