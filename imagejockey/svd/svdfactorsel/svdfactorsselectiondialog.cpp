#include "svdfactorsselectiondialog.h"
#include "ui_svdfactorsselectiondialog.h"
#include "svdfactorsselectionchartview.h"

#include <QLineSeries>
#include <QValueAxis>

SVDFactorsSelectionDialog::SVDFactorsSelectionDialog(const std::vector<double> & weights,
                                                     bool deleteSelfOnClose,
                                                     QWidget *parent) :
    QDialog(parent),
	ui(new Ui::SVDFactorsSelectionDialog),
	m_weights( weights ),
    m_factorsSelChartView( nullptr ),
    m_numberOfFactors( 0 )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose, deleteSelfOnClose);

    setWindowTitle( "SVD factors selection" );

	//create and fill a data series object for the chart
	QtCharts::QLineSeries *series = new QtCharts::QLineSeries();
	std::vector<double>::const_iterator it = m_weights.cbegin();
	double cumulative = 0.0;
	for(uint i = 0; it != m_weights.cend(); ++it, ++i){
		cumulative += *it;
		series->append( i+1, cumulative * 100 );
	}

	//create a new chart object using the data series
	QtCharts::QChart *chart = new QtCharts::QChart();
	chart->legend()->hide();
	chart->addSeries(series);
	chart->createDefaultAxes();
	chart->setTitle("SVD factor cumulative information content curve");
	chart->axisY( series )->setMax(100.0);
	chart->axisY( series )->setMin(0.0);
	chart->axisY( series )->setTitleText("%");
	QtCharts::QValueAxis *axisX = new QtCharts::QValueAxis();
	axisX->setLabelFormat("%.0f");
	chart->setAxisX( axisX, series );
	chart->axisX( series )->setTitleText("Factor #");
	chart->setAcceptHoverEvents( true );

	//create a chart view widget passing the chart object created
	m_factorsSelChartView = new SVDFactorsSelectionChartView( chart, series );
	m_factorsSelChartView->setRenderHint( QPainter::Antialiasing );
	ui->layoutMain->addWidget( m_factorsSelChartView );

    //to be notified of user selection.
    connect( m_factorsSelChartView, SIGNAL(onNumberOfFactorsSelected(int)),
             this, SLOT(onNumberOfFactorsSelected(int)) );
    connect( m_factorsSelChartView, SIGNAL(onNumberOfFactorsSelected(int)),
             this, SIGNAL(numberOfFactorsSelected(int)) );
}

SVDFactorsSelectionDialog::~SVDFactorsSelectionDialog()
{
    delete ui;
}

void SVDFactorsSelectionDialog::onGetAllFactors()
{
    m_numberOfFactors = m_weights.size();
    emit numberOfFactorsSelected( m_numberOfFactors );
    accept();
}

void SVDFactorsSelectionDialog::onGetAllFactorsUpTo100()
{
    //get the factor number that reaches 100% of information content
    std::vector<double>::const_iterator it = m_weights.cbegin();
    double cumulative = 0.0;
    uint i = 1;
    for(i = 1; it != m_weights.cend(); ++it, ++i){
        cumulative += *it;
        if( cumulative > 0.99999 )
            break;
    }
    m_numberOfFactors = i;
    emit numberOfFactorsSelected( m_numberOfFactors );
    accept();
}

