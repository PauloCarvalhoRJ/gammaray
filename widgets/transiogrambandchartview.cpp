#include "transiogrambandchartview.h"

#include <cmath>

#include <QtCharts/QLineSeries>
#include <QtCharts/QAbstractAxis>
#include <QtCharts/QValueAxis>

TransiogramBandChartView::TransiogramBandChartView( QtCharts::QChart *chart,
                                                    TransiogramType type,
                                                    double hMax,
                                                    QtCharts::QValueAxis *axisX,
                                                    QtCharts::QValueAxis *axisY,
                                                    QString headFaciesName,
                                                    QString tailFaciesName,
                                                    QWidget *parent) :
    TransiogramChartView( chart, type, hMax, axisX, axisY, headFaciesName, tailFaciesName, parent ),
    m_range2( -1.0 ),
    m_sill2( -1.0 ),
    m_whichModel( WhichTransiogramModelOfTheBand::FIRST ),
    m_seriesTransiogramModel2( nullptr ),
    m_modelVisible2( true )
{
}

void TransiogramBandChartView::setModel2Parameters(double range2, double sill2)
{
    m_range2 = range2;
    m_sill2 = sill2;
    updateModelSeries2();
}

QtCharts::QLineSeries *TransiogramBandChartView::getSeriesTransiogramModel2() const
{
    return m_seriesTransiogramModel2;
}

void TransiogramBandChartView::setModel2Visible(bool value)
{
    m_modelVisible2 = value;
    updateModelSeries2();
}

void TransiogramBandChartView::mouseMoveEvent(QMouseEvent *event)
{
    updateCrossHairsPosition( event );

    //update one of the models' parameters and redraw one of the models' curve while user drags the mouse
    if( m_mouseDown ){
        double dragged_h = m_chart->mapToValue(event->pos()).x();
        double dragged_p = m_chart->mapToValue(event->pos()).y();
        if( m_whichModel == WhichTransiogramModelOfTheBand::FIRST ){
            m_range = dragged_h;
            m_sill  = dragged_p;
            updateModelSeries();
        }else if( m_whichModel == WhichTransiogramModelOfTheBand::SECOND ){
            m_range2 = dragged_h;
            m_sill2  = dragged_p;
            updateModelSeries2();
        }
    }
}

void TransiogramBandChartView::mousePressEvent(QMouseEvent *event)
{
    //Forward the event to the superclass.
    TransiogramChartView::mousePressEvent(event);

    //Get h (separation) and p (probability) corresponding to the picked location.
    double picked_h = m_chart->mapToValue(event->pos()).x();
    double picked_p = m_chart->mapToValue(event->pos()).y();

    //Get the probability for the picked h according to the 1st model.
    double model1_p = GeostatsUtils::getTransiogramProbability( m_type,
                                                                VariogramStructureType::SPHERIC,
                                                                picked_h,
                                                                m_range,
                                                                m_sill );

    //Get the probability for the picked h according to the 2nd model.
    double model2_p = GeostatsUtils::getTransiogramProbability( m_type,
                                                                VariogramStructureType::SPHERIC,
                                                                picked_h,
                                                                m_range2,
                                                                m_sill2 );

    //Decide which model curve is closer to the picked location.
    //The closest one will be edited in mouseMoveEvent().
    double dp1 = std::abs( model1_p - picked_p );
    double dp2 = std::abs( model2_p - picked_p );
    if( dp1 < dp2 )
        m_whichModel = WhichTransiogramModelOfTheBand::FIRST;
    else
        m_whichModel = WhichTransiogramModelOfTheBand::SECOND;
}

void TransiogramBandChartView::updateModelSeries2()
{
    using namespace QtCharts;

    if( m_range2 >= 0.0 && m_sill2 >= 0.0 ){

        if( m_seriesTransiogramModel2 ){
            m_chart->removeSeries( m_seriesTransiogramModel2 );
            delete m_seriesTransiogramModel2;
            m_seriesTransiogramModel2 = nullptr;
        }

        //create a data series (line in the chart) for the 2nd transiogram model
        m_seriesTransiogramModel2 = new QLineSeries();
        {
            //add the first point, the transriogram value at h = 0.
            //for auto-transiograms, its value is 1.0
            //for cross-transiograms, its value is 0.0
            // see "Transiograms for Characterizing Spatial Variability of Soil Classes", - Li, W. (2007)
            if( m_type == TransiogramType::AUTO_TRANSIOGRAM )
                m_seriesTransiogramModel2->append( 0.0, 1.0 );
            else
                m_seriesTransiogramModel2->append( 0.0, 0.0 );

            if( m_modelVisible2 ) {
                //for each separation h
                double dh = m_hMax / 30.0;
                for( double h = 0.0; h < m_hMax; h += dh ){
                    double theorethicalValue = GeostatsUtils::getTransiogramProbability( m_type,
                                                                                         VariogramStructureType::SPHERIC,
                                                                                         h,
                                                                                         m_range2,
                                                                                         m_sill2 );
                    m_seriesTransiogramModel2->append( h, theorethicalValue );
                }
            }

            QPen pen( QRgb(0x008000) ); //paint the 2nd variogram in green.
            pen.setWidth( 1 );
            m_seriesTransiogramModel2->setPen( pen );
        }

        m_chart->addSeries( m_seriesTransiogramModel2 );
        m_chart->setAxisX( m_axisX, m_seriesTransiogramModel2 );
        m_chart->setAxisY( m_axisY, m_seriesTransiogramModel2 );
    }

    //notify possibly monitoring clients of changes to the 2nd theoretical transiogram parameters
    emit model2WasUpdated();
}
