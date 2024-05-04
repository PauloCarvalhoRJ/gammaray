#include "linechartwidget.h"
#include "ui_linechartwidget.h"

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QAbstractAxis>
#include <QLayout>

LineChartWidget::LineChartWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LineChartWidget),
    m_sharedYaxis(false)
{
    ui->setupUi(this);

    //creates a series objects, which will hold the chart data
    m_series = new QtCharts::QLineSeries();

    //create the chart object
    m_chart = new QtCharts::QChart();
    m_chart->addSeries(m_series);
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);

    //add the horizontal axis
    m_axisX = new QtCharts::QValueAxis();
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_series->attachAxis(m_axisX);
    m_axisX->setTickCount(11);

    //create and adds the chart view widget
    m_chartView = new QtCharts::QChartView( m_chart );
    m_chartView->setRenderHint(QPainter::Antialiasing);
    this->layout()->addWidget( m_chartView );
}

LineChartWidget::~LineChartWidget()
{
    delete ui;
}

void LineChartWidget::setData(const std::vector<std::vector<double> > &data,
                              int indexForXAxis,
                              bool clearCurrentCurves,
                              const std::map<uint8_t, QString>& yVariablesCaptions,
                              const std::map<uint8_t, QString>& yVariablesYaxisTitles,
                              const std::map<uint8_t, QColor>&  yVariablesColors,
                              const std::map<uint8_t, QPen>&    yVariablesStyles )
{
    // Does nothing if the input data table is empty.
    if( data.empty() )
        return;

    // Define limits for the X axis.
    double minX = std::numeric_limits<double>::max();
    double maxX = -std::numeric_limits<double>::max();

    // Clears all current data series.
    if( clearCurrentCurves )
        m_chart->removeAllSeries();

    // Initialize the limits for the Y axes (one per dependent variable or a global Y axis).
    double minY =  std::numeric_limits<double>::max();
    double maxY = -std::numeric_limits<double>::max();

    // There may be one Y axis for all Y variables or one axis per Y variables
    QtCharts::QValueAxis *axisY = nullptr;

    // Create a number of data series and axes. One series per dependent variable.
    for( int iSeries = 0; iSeries < data[0].size(); ++iSeries ){
        if( iSeries != indexForXAxis ){

            //if one Y axis per Y variable is enabled...
            if( ! m_sharedYaxis ) {
                // ...reset the limits for the Y axis (one per dependent variable).
                minY =  std::numeric_limits<double>::max();
                maxY = -std::numeric_limits<double>::max();
            }

            // Create a data series
            QtCharts::QLineSeries* series = new QtCharts::QLineSeries();

            // tries to set a caption for the current series (user may not have set one)
            try {
                series->setName( yVariablesCaptions.at( iSeries ) );
            } catch (...){}

            // tries to set a line style for the current series (user may not have set one)
            try {
                series->setPen( yVariablesStyles.at( iSeries ) );
            } catch (...){}

            // tries to set a color for the current series (user may not have set one)
            try {
                series->setColor( yVariablesColors.at( iSeries ) );
            } catch (...){}

            // For each sample...
            for( const std::vector< double > sample : data ){
                // Get the independent variable.
                double valueX = sample[ indexForXAxis ];
                // Update X value limits.
                minX = std::min( minX, valueX );
                maxX = std::max( maxX, valueX );
                // Get the dependent variable.
                double valueY = sample[ iSeries ];
                // Update Y value limits for the series.
                minY = std::min( minY, valueY );
                maxY = std::max( maxY, valueY );
                // Apend the XY value pair to the series.
                series->append( valueX, valueY );
            } //for each sample

            m_chart->addSeries( series );
            series->attachAxis( m_axisX );

            //create the single Y axis or a new Y axis per dependent variable.
            if( ! m_sharedYaxis || ! axisY ){
                //remove any previously added Y axes
                for( QtCharts::QAbstractAxis* axis : m_chart->axes( Qt::Vertical ) )
                    m_chart->removeAxis( axis );
                axisY = new QtCharts::QValueAxis();
                axisY->setTickCount(11);
                m_chart->addAxis(axisY, Qt::AlignLeft);
            }
            axisY->setRange( minY, maxY );
            series->attachAxis( axisY );

            // tries to set a Y-axis title for the current series (user may not have set one)
            try {
                axisY->setTitleText( yVariablesYaxisTitles.at( iSeries ) );
            } catch (...){}

        } //if to skip the independent variable, which is assigned to X-axis
    } //

    //Update the X axis.
    m_axisX->setRange( minX, maxX );

}

void LineChartWidget::setChartTitle(const QString chartTitle)
{
    m_chart->setTitle( chartTitle );
}

void LineChartWidget::setXaxisCaption(const QString caption)
{
    m_axisX->setTitleText( caption );
}

void LineChartWidget::setLegendVisible(const bool value)
{
    if( value )
        m_chart->legend()->show();
    else
        m_chart->legend()->hide();
}
