#include "transiogramchartview.h"
#include <QLineSeries>
#include <QRubberBand>
#include <QValueAxis>
#include <iostream>
#include "geostats/geostatsutils.h"

TransiogramChartView::TransiogramChartView(QtCharts::QChart *chart,
                                           TransiogramType type,
                                           double hMax,
                                           QtCharts::QValueAxis *axisX,
                                           QtCharts::QValueAxis *axisY,
                                           QString headFaciesName,
                                           QString tailFaciesName,
                                           QWidget *parent) :
    QtCharts::QChartView(chart, parent),
    m_chart( chart ),
    m_rubberBandH( nullptr ),
    m_rubberBandV( nullptr ),
    m_range( -1.0 ),
    m_sill( -1.0 ),
    m_type( type ),
    m_axisX( axisX ),
    m_axisY( axisY ),
    m_hMax( hMax ),
    m_seriesTransiogramModel( nullptr ),
    m_mouseDown( false ),
    m_headFaciesName( headFaciesName ),
    m_tailFaciesName( tailFaciesName )
{
    //enable mouse move/hover events
    this->setMouseTracking( true );

    //create objects to make a crosshairs
    m_rubberBandH = new QRubberBand(QRubberBand::Line, this);
    m_rubberBandV = new QRubberBand(QRubberBand::Line, this);
}

void TransiogramChartView::mouseMoveEvent(QMouseEvent *event)
{
    //update the crosshairs position constrained to the plot area in the widget
    int x = std::max<int>( event->pos().x(), static_cast<int>( m_chart->plotArea().x() ) );
    x = std::min<int>( x, static_cast<int>( m_chart->plotArea().x() + m_chart->plotArea().width() ) );
    int y = std::max<int>( event->pos().y(), static_cast<int>( m_chart->plotArea().y() ) );
    y = std::min<int>( y, static_cast<int>( m_chart->plotArea().y() + m_chart->plotArea().height() ) );
    m_rubberBandH->setGeometry( m_chart->plotArea().x(), y,   m_chart->plotArea().width(),  1 );
    m_rubberBandV->setGeometry( x, m_chart->plotArea().y(),   1, m_chart->plotArea().height() );

    //update model parameters and redraw model curve while user drags the mouse
    if( m_mouseDown ){
        m_range = m_chart->mapToValue(event->pos()).x();
        m_sill  = m_chart->mapToValue(event->pos()).y();
        updateModelSeries();
    }
}

void TransiogramChartView::showOrHideCrossHairs()
{
    if( m_mouseDown ){
        m_rubberBandH->show();
        m_rubberBandV->show();
    } else {
        m_rubberBandH->hide();
        m_rubberBandV->hide();
    }
}

void TransiogramChartView::updateModelSeries()
{
    using namespace QtCharts;

    if( m_range >= 0.0 && m_sill >= 0.0 ){

        if( m_seriesTransiogramModel ){
            m_chart->removeSeries( m_seriesTransiogramModel );
            delete m_seriesTransiogramModel;
        }

        //create a data series (line in the chart) for the transiogram model
        m_seriesTransiogramModel = new QLineSeries();
        {
            //add the first point, the transriogram value at h = 0.
            //for auto-transiograms, its value is 1.0
            //for cross-transiograms, its value is 0.0
            // see "Transiograms for Characterizing Spatial Variability of Soil Classes", - Li, W. (2007)
            if( m_type == TransiogramType::AUTO_TRANSIOGRAM )
                m_seriesTransiogramModel->append( 0.0, 1.0 );
            else
                m_seriesTransiogramModel->append( 0.0, 0.0 );

            //for each separation h
            double dh = m_hMax / 30.0;
            for( double h = 0.0; h < m_hMax; h += dh ){
                double theorethicalValue;
                if( m_type == TransiogramType::AUTO_TRANSIOGRAM )
                    theorethicalValue = 1.0 - GeostatsUtils::getGamma(
                                                   VariogramStructureType::SPHERIC, h, m_range, 1.0 - m_sill );
                else         //for cross-transiograms
                    theorethicalValue = GeostatsUtils::getGamma(
                                                   VariogramStructureType::SPHERIC, h, m_range, m_sill );
                m_seriesTransiogramModel->append( h, theorethicalValue );
            }

            QPen pen( QRgb(0x0000FF) );
            pen.setWidth( 1 );
            m_seriesTransiogramModel->setPen( pen );
        }

        m_chart->addSeries( m_seriesTransiogramModel );
        m_chart->setAxisX( m_axisX, m_seriesTransiogramModel );
        m_chart->setAxisY( m_axisY, m_seriesTransiogramModel );
    }

    //notify possibly monitoring clients of changes to the theoretical transiogram parameters
    emit updated();
}

void TransiogramChartView::mousePressEvent(QMouseEvent *event)
{
    m_mouseDown = true;
    showOrHideCrossHairs();
}

void TransiogramChartView::mouseReleaseEvent(QMouseEvent *event)
{
    m_mouseDown = false;
    showOrHideCrossHairs();
}

void TransiogramChartView::setModelParameters(double range, double sill)
{
    m_range = range;
    m_sill = sill;
    updateModelSeries();
}
