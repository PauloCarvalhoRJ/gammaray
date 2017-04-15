#include "softindicatorcalibplot.h"
#include <QEvent>
#include <qwt_scale_widget.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_layout.h>

#include "softindicatorcalibcanvaspicker.h"

SoftIndicatorCalibPlot::SoftIndicatorCalibPlot(QWidget *parent) :
    QwtPlot(parent)
{
    setTitle( "Soft indicator calibration." );

    QwtPlotGrid *grid = new QwtPlotGrid;
    grid->setMajorPen( Qt::white, 0, Qt::DotLine );
    grid->attach( this );

    // axes
    setAxisScale( QwtPlot::xBottom, 0.0, 100.0 );
    setAxisScale( QwtPlot::yLeft, 0.0, 100.0 );

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
    ( void ) new SoftIndicatorCalibCanvasPicker( this );
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

    insertCurve( Qt::Horizontal, 20.0);
}

void SoftIndicatorCalibPlot::insertCurve(int axis, double base)
{
    Qt::Orientation o;
    if ( axis == yLeft || axis == yRight )
        o = Qt::Horizontal;
    else
        o = Qt::Vertical;

    QRgb rgb = static_cast<QRgb>( rand() );
    insertCurve( o, QColor( rgb ), base );
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
        //delete *it;
    }
    m_curves.clear();
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
