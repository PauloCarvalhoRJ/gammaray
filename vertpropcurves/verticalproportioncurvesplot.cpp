#include "verticalproportioncurvesplot.h"

#include <qwt_plot_grid.h>
#include <qwt_scale_widget.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>
#include <qwt_plot_intervalcurve.h>

#include "util.h"

VerticalProportionCurvesPlot::VerticalProportionCurvesPlot( QWidget *parent ) :
    QwtPlot( parent ),
    m_nCurves(2),
    m_nPoints(11),
    m_handleSize(8),
    m_legendIconSize(16)
{
    setTitle( "Vertical proportion curve." );

    //set icon and curves handles sizes depending on display resolution
    if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI ) {
        m_handleSize = 16;
        m_legendIconSize = 32;
    }

    QwtPlotGrid *grid = new QwtPlotGrid;
    grid->setMajorPen( Qt::black, 0, Qt::DotLine );
    grid->attach( this );

    // axes
    setAxisScale( QwtPlot::xBottom, 0.0, 100.0 );
    setAxisTitle( QwtPlot::xBottom, QString( "proportion (%)" ) );
    setAxisScale( QwtPlot::yLeft, 0.0, 100.0 );
    setAxisTitle( QwtPlot::yLeft, QString( "base-top depth fraction (%)" ) );

    // Avoid jumping when label with 3 digits
    // appear/disappear when scrolling vertically
    QwtScaleDraw *sd = axisScaleDraw( QwtPlot::yLeft );
    sd->setMinimumExtent( sd->extent( axisWidget( QwtPlot::yLeft )->font() ) );

    plotLayout()->setAlignCanvasToScales( true );

    insertCurve( Qt::Vertical, Qt::yellow, 30.0, QString() );
    insertCurve( Qt::Vertical, Qt::white, 70.0, QString() );

    replot();
}

void VerticalProportionCurvesPlot::setNumberOfCurves(size_t number)
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

void VerticalProportionCurvesPlot::insertCurve(int axis, double base)
{
    Qt::Orientation o;
    if ( axis == yLeft || axis == yRight )
        o = Qt::Horizontal;
    else
        o = Qt::Vertical;

    insertCurve( o, Qt::black, base, QString() );
    replot();
}

void VerticalProportionCurvesPlot::insertCurve(Qt::Orientation o, const QColor &c, double base, QString label)
{
    QwtPlotCurve *curve = new QwtPlotCurve( label );

    curve->setPen( c );
    curve->setSymbol( new QwtSymbol( QwtSymbol::Ellipse,
        Qt::gray, c, QSize( m_handleSize, m_handleSize ) ) );

    double x[m_nPoints];
    double y[m_nPoints];

    //min and max in both axes are always 0.0 and 100.0 (%)
    double step = 100.0 / (m_nPoints - 1);

    //min in both axes is always 0.0 (%)
    double dataMin = 0.0;

    for ( uint i = 0; i < m_nPoints; i++ )
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

    curve->setSamples( x, y, m_nPoints );
    curve->attach( this );

    //put legend icon if label text is set
    if( ! label.isEmpty() )
        curve->setLegendIconSize( QSize( m_legendIconSize, m_legendIconSize ) );
    else
        curve->setLegendIconSize( QSize( 0, 0 ) );

    m_curves.push_back( curve );
}

void VerticalProportionCurvesPlot::clearCurves()
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

void VerticalProportionCurvesPlot::clearFillAreas()
{
    std::vector<QwtPlotIntervalCurve*>::iterator it = m_fillAreas.begin();
    for(; it != m_fillAreas.end(); ++it){
        (*it)->detach( );
        //delete *it; //assuming autoDelete is on for this QwtPlot
    }
    m_fillAreas.clear();
}
