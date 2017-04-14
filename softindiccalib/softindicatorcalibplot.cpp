#include "softindicatorcalibplot.h"
#include <QEvent>
#include <qwt_scale_widget.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_layout.h>

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

    // ------------------------------------
    // We add a color bar to the left axis
    // ------------------------------------

    QwtScaleWidget *scaleWidget = axisWidget( yLeft );
    scaleWidget->setMargin( 10 ); // area for the color bar
    //d_colorBar = new ColorBar( Qt::Vertical, scaleWidget );
    //d_colorBar->setRange( Qt::red, Qt::darkBlue );
    //d_colorBar->setFocusPolicy( Qt::TabFocus );

    //connect( d_colorBar, SIGNAL( selected( const QColor & ) ),
    //    SLOT( setCanvasColor( const QColor & ) ) );

    // we need the resize events, to lay out the color bar
    scaleWidget->installEventFilter( this );

    // ------------------------------------
    // We add a wheel to the canvas
    // ------------------------------------

    //d_wheel = new QwtWheel( canvas() );
    //d_wheel->setOrientation( Qt::Vertical );
    //d_wheel->setRange( -100, 100 );
    //d_wheel->setValue( 0.0 );
    //d_wheel->setMass( 0.2 );
    //d_wheel->setTotalAngle( 4 * 360.0 );

    //connect( d_wheel, SIGNAL( valueChanged( double ) ),
    //    SLOT( scrollLeftAxis( double ) ) );

    // we need the resize events, to lay out the wheel
    canvas()->installEventFilter( this );

    //d_colorBar->setWhatsThis(
    //    "Selecting a color will change the background of the plot." );
    scaleWidget->setWhatsThis(
        "Selecting a value at the scale will insert a new curve." );
    //d_wheel->setWhatsThis(
    //    "With the wheel you can move the visible area." );
    axisWidget( xBottom )->setWhatsThis(
        "Selecting a value at the scale will insert a new curve." );
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

    double x[10];
    double y[sizeof( x ) / sizeof( x[0] )];

    for ( uint i = 0; i < sizeof( x ) / sizeof( x[0] ); i++ )
    {
        double v = 5.0 + i * 10.0;
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

    curve->setSamples( x, y, sizeof( x ) / sizeof( x[0] ) );
    curve->attach( this );

}
