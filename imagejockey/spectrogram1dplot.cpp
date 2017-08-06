#include "spectrogram1dplot.h"

#include <qwt_plot_canvas.h>
#include <qwt_scale_widget.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>

#include "domain/attribute.h"


///================================THE GRID PATTERN FOR THE 1D SPECTROGRAM===============
class Spectrogram1DGrid: public QwtPlotGrid
{
public:
    Spectrogram1DGrid()
    {
        enableXMin( true );
        enableYMin( true );
        setMajorPen( Qt::green, 0, Qt::DotLine );
        setMinorPen( Qt::darkGreen, 0, Qt::DotLine );
    }

    virtual void updateScaleDiv(
            const QwtScaleDiv &xScaleDiv,
            const QwtScaleDiv &yScaleDiv )
    {
        QwtScaleDiv scaleDiv( xScaleDiv.lowerBound(),
                              xScaleDiv.upperBound() );
        scaleDiv.setTicks( QwtScaleDiv::MinorTick,
            xScaleDiv.ticks( QwtScaleDiv::MinorTick ) );
        scaleDiv.setTicks( QwtScaleDiv::MajorTick,
            xScaleDiv.ticks( QwtScaleDiv::MediumTick ) );

        QwtScaleDiv scaleDivY( yScaleDiv.lowerBound(),
                              yScaleDiv.upperBound() );
        scaleDivY.setTicks( QwtScaleDiv::MinorTick,
            yScaleDiv.ticks( QwtScaleDiv::MinorTick ) );
        scaleDivY.setTicks( QwtScaleDiv::MajorTick,
            yScaleDiv.ticks( QwtScaleDiv::MediumTick ) );

        QwtPlotGrid::updateScaleDiv( xScaleDiv, scaleDivY );
    }
};

///=============================THE SPECTROGRAM1DPLOT ITSELF===============================

Spectrogram1DPlot::Spectrogram1DPlot(QWidget *parent) :
    QwtPlot( parent ),
    m_at( nullptr ),
    m_curve( new QwtPlotCurve() )
{
    QwtPlotCanvas *canvas = new QwtPlotCanvas();
    canvas->setPalette( Qt::black );
    setCanvas( canvas );

    // grid
    QwtPlotGrid *grid = new Spectrogram1DGrid();
    grid->attach( this );

    // plot curve
    m_curve->setRenderHint( QwtPlotItem::RenderAntialiased );
    m_curve->setStyle( QwtPlotCurve::Dots );
    m_curve->setPen( Qt::green, 5 );
    m_curve->attach( this );

    //axes text styles
    setAxisTitle( QwtPlot::yLeft, "<span style=\" font-size:7pt;\">dB</span>");
    setAxisTitle( QwtPlot::xBottom, "<span style=\" font-size:7pt;\">frequency</span>");
    QwtScaleWidget* xaxisw = axisWidget( Axis::yLeft );
    xaxisw->setStyleSheet("font: 7pt;");
    QwtScaleWidget* yaxisw = axisWidget( Axis::xBottom );
    yaxisw->setStyleSheet("font: 7pt;");
}

void Spectrogram1DPlot::setAttribute(Attribute *at)
{
    m_at = at;


    QVector<QPointF> samples;
    samples.push_back( QPointF(100,100) );
    samples.push_back( QPointF(300,150) );
    samples.push_back( QPointF(500,600) );
    samples.push_back( QPointF(700,400) );
    m_curve->setSamples( samples );
    replot();
}
