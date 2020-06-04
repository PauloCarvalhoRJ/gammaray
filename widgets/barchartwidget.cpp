#include "barchartwidget.h"
#include "ui_barchartwidget.h"

#include <QtCharts/QChartView>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarSet>

#include <QLayout>

BarChartWidget::BarChartWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BarChartWidget)
{
    ui->setupUi(this);

    //creates a series objects, which will hold the chart data
    m_series = new QtCharts::QBarSeries();

    //create the chart object
    m_chart = new QtCharts::QChart();
    m_chart->addSeries(m_series);
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);

    //define the horizontal axis
    QtCharts::QBarCategoryAxis *axisX = new QtCharts::QBarCategoryAxis();
    axisX->append( QStringList("") );
    m_chart->addAxis(axisX, Qt::AlignBottom);
    m_series->attachAxis(axisX);

    //define the vertical axis
    QtCharts::QValueAxis *axisY = new QtCharts::QValueAxis();
    axisY->setRange(0,15);
    m_chart->addAxis(axisY, Qt::AlignLeft);
    m_series->attachAxis(axisY);

    //create and adds the chart view widget
    m_chartView = new QtCharts::QChartView( m_chart );
    m_chartView->setRenderHint(QPainter::Antialiasing);
    this->layout()->addWidget( m_chartView );
}

BarChartWidget::~BarChartWidget()
{
    delete ui;
}

void BarChartWidget::setChartTitle(const QString title)
{
    m_chart->setTitle( title );
}

void BarChartWidget::addBar(const QString categoryName,
                            double value,
                            const QColor color )
{
    QtCharts::QBarSet *set = new QtCharts::QBarSet( categoryName );
    *set << value;
    set->setColor( color );
    m_series->append(set);
}

void BarChartWidget::setMaxY(double value)
{
    m_chart->axisY( m_series )->setMax( value );
}

void BarChartWidget::setAxisYLabel(const QString label)
{
    m_chart->axisY( m_series )->setTitleText( label );
}

void BarChartWidget::setAxisXLabel(const QString label)
{
    m_chart->axisX( m_series )->setTitleText( label );
}
