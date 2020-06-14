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

    // Get the number of dependent variables.
    int nVarsY = data[0].size() - 1;

    // Create a number of Y data series and axes.

    // For each sample...
    for( const std::vector< double > sample : data ){
        // Get the independent variable.
        double valueX = sample[ indexForXAxis ];
        // Update X value limits.
        minX = std::min( minX, valueX );
        maxX = std::max( maxX, valueX );
        // Get the dependent variable(s).
        for( int iValue = 0; iValue < sample.size(); ++iValue ){
            if( iValue != indexForXAxis ){

            }
        }
    }

    //Update the X axis.
    m_axisX->setRange( minX, maxX );

}
