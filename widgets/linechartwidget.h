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

    /**
     * Displays a multivariate line chart with a shared X-axis.
     * The Y-axis can be shared or one per Y variable
     * @param swapXY Sets whether to display X values to the left and Y values to the bottom.
     * @see setSharedYaxis()
     */
    explicit LineChartWidget( QWidget *parent = nullptr, bool swapXY = false );
    ~LineChartWidget();

    /**
     * Updates the chart with the passed data table.
     * The outer vector corresponds to samples along the X axis.
     * Each inner vector corresponds to a sample (consisting of at least two doubles for a simple XY chart).
     * @param indexForXAxis The index of element in the inner vectors corresponding to the
     *                      independent variable, that is, the values in the X-axis.  The other
     *                      values will be considered Y variables.
     * @param yVariablesCaptions Sets the captions for each curve legend.  If a caption is not set for a given curve,
     *                           the legend will be displayed with a blank caption.  The uint8_t value is the index of
     *                           the Y value in the inner vectors in data vector.
     * @param yVariablesYaxisTitles Sets the y-axis titles for each curve.  If a title is not set for a given curve,
     *                              the title will be absent.  The uint8_t value is the index of
     *                              the Y value in the inner vectors in data vector.  If setSharedYaxis(true) was called,
     *                              then the title of the curve with highest index will be used for the shared Y-axis (if not set,
     *                              it'll be blank even if the other titles were set).
     * @param yVariablesColors  Sets the colors for each curve.  If a color is not set for a given curve,
     *                          it'll will be displayed with a default color.  The uint8_t value is the index of
     *                          the Y value in the inner vectors in data vector.
     */
    void setData( const std::vector< std::vector< double > >& data,
                  int indexForXAxis,
                  const std::map<uint8_t, QString>& yVariablesCaptions    = {},
                  const std::map<uint8_t, QString>& yVariablesYaxisTitles = {},
                  const std::map<uint8_t, QColor>&  yVariablesColors      = {} );

    /**
     * Enables/disables whether all Y series share the same vertical axis in the chart.
     * This setting has effect only in a following call to setData(). Default is false.
     */
    void setSharedYaxis( bool value ){ m_sharedYaxis = value; }

    /**
     * Sets the chart's title.  Takes effect upon calling.
     */
    void setChartTitle( const QString chartTitle );

    /**
     * Sets the x-axis's caption text.  Takes effect upon calling.
     */
    void setXaxisCaption( const QString caption );

    /**
     * Shows or hides the chart legend (default is true).  Takes effect upon calling.
     */
    void setLegendVisible( const bool value );

private:
    Ui::LineChartWidget *ui;

    QtCharts::QLineSeries* m_series;
    QtCharts::QChart* m_chart;
    QtCharts::QChartView* m_chartView;
    QtCharts::QValueAxis *m_axisX;

    bool m_sharedYaxis;
    bool m_swapXY;

};

#endif // LINECHARTWIDGET_H
