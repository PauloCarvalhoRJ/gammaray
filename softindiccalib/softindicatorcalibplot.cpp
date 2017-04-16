#include "softindicatorcalibplot.h"
#include <QEvent>
#include <qwt_scale_widget.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_intervalcurve.h>

#include "softindicatorcalibcanvaspicker.h"

SoftIndicatorCalibPlot::SoftIndicatorCalibPlot(QWidget *parent) :
    QwtPlot(parent),
    m_nCurves(2)
{
    setTitle( "Soft indicator calibration." );

    QwtPlotGrid *grid = new QwtPlotGrid;
    grid->setMajorPen( Qt::black, 0, Qt::DotLine );
    grid->attach( this );

    // axes
    setAxisScale( QwtPlot::xBottom, 0.0, 100.0 );
    setAxisScale( QwtPlot::yLeft, 0.0, 100.0 );

    setAxisTitle( QwtPlot::yLeft,
        QString( "Probability (%)" ) );

    // Avoid jumping when label with 3 digits
    // appear/disappear when scrolling vertically

    QwtScaleDraw *sd = axisScaleDraw( QwtPlot::yLeft );
    sd->setMinimumExtent( sd->extent( axisWidget( QwtPlot::yLeft )->font() ) );

    plotLayout()->setAlignCanvasToScales( true );

    insertCurve( Qt::Horizontal, Qt::yellow, 30.0 );
    insertCurve( Qt::Horizontal, Qt::white, 70.0 );

    replot();

    // we need the resize events, to lay out the wheel
    canvas()->installEventFilter( this );

    axisWidget( xBottom )->setWhatsThis(
        "Selecting a value at the scale will insert a new curve." );

    // The canvas picker handles all mouse and key
    // events on the plot canvas
    //( void ) new SoftIndicatorCalibCanvasPicker( this );
    SoftIndicatorCalibCanvasPicker* siccp = new SoftIndicatorCalibCanvasPicker( this ); //TODO: delete?
    //To be notified when a calibration curve is edited by the used.
    connect( siccp, SIGNAL(curveChanged()), this, SLOT(onCurveChanged()) );
}

bool SoftIndicatorCalibPlot::eventFilter(QObject *object, QEvent *e)
{
    if ( e->type() == QEvent::Resize )
    {
        if ( object == axisWidget( yLeft ) )
        {
        }
        if ( object == canvas() )
        {
        }
    }

    return QwtPlot::eventFilter( object, e );
}

void SoftIndicatorCalibPlot::transferData(std::vector<double> &source)
{
    //remove current calibration curves
    clearCurves();

    //get the data
    m_data = std::move( source );

    //update horizontal axis scale
    setAxisScale( QwtPlot::xBottom, getDataMin(), getDataMax() );

    //set initial spacing between the curves
    double step = 100.0 / ( m_nCurves + 1 );

    //add the curves
    for( size_t i = 0; i < m_nCurves; ++i){
        insertCurve( Qt::Horizontal, step * (i+1));
    }
}

void SoftIndicatorCalibPlot::setNumberOfCurves(size_t number)
{
    m_nCurves = number;

    //remove current calibration curves
    clearCurves();

    //set the initial spacing between the curves
    double step = 100.0 / ( m_nCurves + 1 );

    //add the curves
    for( size_t i = 0; i < m_nCurves; ++i){
        insertCurve( Qt::Horizontal, step * (i+1));
    }
}

std::vector<std::vector<double> > SoftIndicatorCalibPlot::getSoftIndicators(SoftIndicatorCalculationMode mode)
{
    std::vector<std::vector<double> > m;
    return m;
}

void SoftIndicatorCalibPlot::setXAxisLabel(QString text)
{
    setAxisTitle( QwtPlot::xBottom, text );
}

void SoftIndicatorCalibPlot::fillColor(const QColor &color, int base_curve)
{
    //does nothing if there is no calibration curve.
    if( m_curves.size() == 0 )
        return;

    //get the number curve points (assumes all curves have the same number of points)
    int nData = m_curves[0]->data()->size();

    //correct for possibly out-of-range base curve index
    if( base_curve > (int)m_curves.size()-1 )
        base_curve = (int)m_curves.size()-1;
    if( base_curve < -1 )
        base_curve = -1;

    //get the index of the top curve
    int top_curve = base_curve + 1;

    //create a x, y_base, y_top sample collection object
    QVector<QwtIntervalSample> intervals( nData );

    //get the y intervals between the target curves
    for ( int i = 0; i < nData; ++i )
    {
        double low;
        double high;
        double x = 0.0;

        if( base_curve >= 0 ){
            low = m_curves[base_curve]->sample(i).y();
            x = m_curves[base_curve]->sample(i).x();
        }else
            low = 0.0;

        if( top_curve < (int)m_curves.size() ){
            high = m_curves[top_curve]->sample(i).y();
            x = m_curves[top_curve]->sample(i).x();
        }else
            high = 100.0;

        intervals[i] = QwtIntervalSample( x, QwtInterval( low, high ) );
    }

    //create and plot a interval curve object
    QwtPlotIntervalCurve *d_intervalCurve = new QwtPlotIntervalCurve( "Range" );
    d_intervalCurve->setRenderHint( QwtPlotItem::RenderAntialiased );
    QColor bg( color );
    bg.setAlpha( 75 );
    d_intervalCurve->setBrush( QBrush( bg ) );
    d_intervalCurve->setStyle( QwtPlotIntervalCurve::Tube );
    d_intervalCurve->setSamples( intervals );
    d_intervalCurve->attach( this );

    //store the pointer of the new interval curve
    m_fillAreas.push_back( d_intervalCurve );
}

void SoftIndicatorCalibPlot::insertCurve(int axis, double base)
{
    Qt::Orientation o;
    if ( axis == yLeft || axis == yRight )
        o = Qt::Horizontal;
    else
        o = Qt::Vertical;

    //QRgb rgb = static_cast<QRgb>( rand() );
    //insertCurve( o, QColor( rgb ), base );
    insertCurve( o, Qt::black, base );
    replot();
}

void SoftIndicatorCalibPlot::insertCurve(Qt::Orientation o, const QColor &c, double base)
{
    QwtPlotCurve *curve = new QwtPlotCurve();

    curve->setPen( c );
    curve->setSymbol( new QwtSymbol( QwtSymbol::Ellipse,
        Qt::gray, c, QSize( 8, 8 ) ) );

    size_t nPoints = 11;
    double x[nPoints];
    double y[nPoints];

    double step = ( getDataMax() - getDataMin() ) / (nPoints - 1);

    double dataMin = getDataMin();

    for ( uint i = 0; i < nPoints; i++ )
    {
        double v = dataMin + i * step;
        if ( o == Qt::Horizontal )
        {
            x[i] = v;
            y[i] = base;
        }
        else
        {
            x[i] = base;
            y[i] = v;
        }
    }

    curve->setSamples( x, y, nPoints );
    curve->attach( this );

    m_curves.push_back( curve );
}

void SoftIndicatorCalibPlot::clearCurves()
{
    std::vector<QwtPlotCurve*>::iterator it = m_curves.begin();
    for(; it != m_curves.end(); ++it){
        (*it)->detach( );
        //delete *it;  //assuming autoDelete is on for this QwtPlot
    }
    m_curves.clear();

    //also clears the filled areas between the curves, if any.
    clearFillAreas();
}

void SoftIndicatorCalibPlot::clearFillAreas()
{
    std::vector<QwtPlotIntervalCurve*>::iterator it = m_fillAreas.begin();
    for(; it != m_fillAreas.end(); ++it){
        (*it)->detach( );
        //delete *it; //assuming autoDelete is on for this QwtPlot
    }
    m_fillAreas.clear();
}

double SoftIndicatorCalibPlot::getDataMax()
{
    if( m_data.size() > 0 ){
        std::vector<double>::iterator it = std::max_element( m_data.begin(), m_data.end() );
        return *it;
    } else {
        return 100.0;
    }
}

double SoftIndicatorCalibPlot::getDataMin()
{
    if( m_data.size() > 0 ){
        std::vector<double>::iterator it = std::min_element( m_data.begin(), m_data.end() );
        return *it;
    } else {
        return 0.0;
    }
}

void SoftIndicatorCalibPlot::onCurveChanged()
{
    STOPPED_HERE;
}
