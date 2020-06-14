#ifndef LINECHARTWIDGET_H
#define LINECHARTWIDGET_H

#include <QWidget>

namespace Ui {
class LineChartWidget;
}

namespace QtCharts {
class QLineSeries;
class QChart;
class QChartView;
class QValueAxis;
}

class LineChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LineChartWidget(QWidget *parent = nullptr);
    ~LineChartWidget();

    /**
     * Updates the chart with the passed data table.
     * The outer vector corresponds to samples along the X axis.
     * The inner vector corresponds to values (at least two for a simple XY chart) of a sample.
     * @param indexForXAxis The index of element in the inner vectors corresponding to the
     *                      independent variable, that is, the values in the X-axis.  The other
     *                      values will be assigned to one or more Y-axes.
     */
    void setData( const std::vector< std::vector< double > >& data,
                  int indexForXAxis );

private:
    Ui::LineChartWidget *ui;

    QtCharts::QLineSeries* m_series;
    QtCharts::QChart* m_chart;
    QtCharts::QChartView* m_chartView;
    QtCharts::QValueAxis *m_axisX;
};

#endif // LINECHARTWIDGET_H
