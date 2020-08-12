#include "linechartwidget.h"
#include "ui_linechartwidget.h"

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QLayout>

LineChartWidget::LineChartWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LineChartWidget)
{
    ui->setupUi(this);

    //creates a series objects, which will hold the chart data
    m_series = new QtCharts::QLineSeries();

    //create the chart object
    m_chart = new QtCharts::QChart();
    m_chart->addSeries(m_series);
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);

    //define the horizontal axis
    m_axisX = new QtCharts::QValueAxis();
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_series->attachAxis(m_axisX);

    //define the vertical axis
    QtCharts::QValueAxis *axisY = new QtCharts::QValueAxis();
    m_chart->addAxis(axisY, Qt::AlignLeft);
    m_series->attachAxis(axisY);

    //create and adds the chart view widget
    m_chartView = new QtCharts::QChartView( m_chart );
    m_chartView->setRenderHint(QPainter::Antialiasing);
    this->layout()->addWidget( m_chartView );
}

LineChartWidget::~LineChartWidget()
{
    delete ui;
}

void LineChartWidget::setData(const std::vector<std::vector<double> > &data, int indexForXAxis)
{
    // Does nothing if the input data table is empty.
    if( data.empty() )
        return;

    // Define limits for the X axis.
    double minX = std::numeric_limits<double>::max();
    double maxX = -std::numeric_limits<double>::max();

    // Clears all current data series.
    m_chart->removeAllSeries();

    // Create a number of data series and axes. One series per dependent variable.
    for( int iSeries = 0; iSeries < data[0].size(); ++iSeries ){
        if( iSeries != indexForXAxis ){

            // Define limits for the Y axis (one per dependent variable).
            double minY = std::numeric_limits<double>::max();
            double maxY = -std::numeric_limits<double>::max();

            // Create a Y-axis
            QtCharts::QValueAxis *axisY = new QtCharts::QValueAxis();

            // Create a series
            QtCharts::QLineSeries* series = new QtCharts::QLineSeries();

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

            //Update the Y axis (one per dependent variable).
            axisY->setRange( minY, maxY );

            m_chart->addSeries( series );
            m_chart->addAxis(axisY, Qt::AlignLeft);
            series->attachAxis( m_axisX );
            series->attachAxis( axisY );

        } //if to skip the independent variable, which is assigned to X-axis
    } //

    //Update the X axis.
    m_axisX->setRange( minX, maxX );

}
