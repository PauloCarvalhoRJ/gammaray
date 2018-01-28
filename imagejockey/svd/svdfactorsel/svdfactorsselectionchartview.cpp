#include "svdfactorsselectionchartview.h"

#include <QLineSeries>
#include "svdfactorsselectionchartcallout.h"

SVDFactorsSelectionChartView::SVDFactorsSelectionChartView(QtCharts::QChart* chart, QtCharts::QLineSeries* series, QWidget * parent) :
	QtCharts::QChartView(chart, parent),
	m_coordX( nullptr ),
	m_coordY( nullptr ),
	m_chart( chart ),
	m_tooltip( nullptr )
{
	//create text elements for probing the chart
	m_coordX = new QGraphicsSimpleTextItem(chart);
	m_coordX->setPos(chart->size().width()/2 - 50, chart->size().height());
	m_coordX->setText("X: ");
	m_coordY = new QGraphicsSimpleTextItem(chart);
	m_coordY->setPos(chart->size().width()/2 + 50, chart->size().height());
	m_coordY->setText("Y: ");

	//capture mouse events triggered by the chart series
	connect(series, &QtCharts::QLineSeries::clicked, this, &SVDFactorsSelectionChartView::keepCallout);
	connect(series, &QtCharts::QLineSeries::hovered, this, &SVDFactorsSelectionChartView::tooltip);

	//enable mouse move/hover events
	this->setMouseTracking( true );
}

void SVDFactorsSelectionChartView::resizeEvent(QResizeEvent *event)
{
	if (scene()) {
		scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));
		 m_chart->resize(event->size());
		 m_coordX->setPos(m_chart->size().width()/2 - 50, m_chart->size().height() - 20);
		 m_coordY->setPos(m_chart->size().width()/2 + 50, m_chart->size().height() - 20);
		 const auto callouts = m_callouts;
		 for (SVDFactorsSelectionChartCallout *callout : callouts)
			 callout->updateGeometry();
	}
	QGraphicsView::resizeEvent(event);
}

void SVDFactorsSelectionChartView::mouseMoveEvent(QMouseEvent *event)
{
	m_coordX->setText(QString("Factor# %1").arg((int)m_chart->mapToValue(event->pos()).x()));
	m_coordY->setText(QString("Info. cont: %1%").arg(m_chart->mapToValue(event->pos()).y()));
    QGraphicsView::mouseMoveEvent(event);
}

int SVDFactorsSelectionChartView::getSelectedNumberOfFactors()
{
    if( m_callouts.size() < 1 )
        return 0;
    //assumes there is just one fixed balloon showing the factor number
    SVDFactorsSelectionChartCallout* callout = m_callouts.back();
    return callout->getFactorNumber();
}

void SVDFactorsSelectionChartView::keepCallout()
{
    //clear the vector to keep just one
    while( ! m_callouts.empty() ){
        delete m_callouts.back();
        m_callouts.pop_back();
    }
    m_callouts.append(m_tooltip);
	m_tooltip = new SVDFactorsSelectionChartCallout(m_chart);
    emit onNumberOfFactorsSelected( getSelectedNumberOfFactors() );
}

void SVDFactorsSelectionChartView::tooltip(QPointF point, bool state)
{
	if (m_tooltip == 0)
		m_tooltip = new SVDFactorsSelectionChartCallout(m_chart);

	if (state) {
        m_tooltip->setFactorNumber( (int)point.x() );
        m_tooltip->setText(QString("Factor# %1 \nInfo. cont: %2% ").arg((int)point.x()).arg(point.y()));
		m_tooltip->setAnchor(point);
		m_tooltip->setZValue(11);
		m_tooltip->updateGeometry();
		m_tooltip->show();
	} else {
		m_tooltip->hide();
	}
}
