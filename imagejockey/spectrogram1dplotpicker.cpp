#include "spectrogram1dplotpicker.h"

#include "spectrogram1dplot.h"

#include <qwt_plot.h>
#include <qwt_symbol.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_directpainter.h>
#include <qwt_plot_canvas.h>

#include <QMouseEvent>
#include <QEvent>
#include <QApplication>

Spectrogram1DPlotPicker::Spectrogram1DPlotPicker(QwtPlotCurve *drawingCurve, QwtPlot *plot) :
    QObject( plot ),
    m_drawingCurve( drawingCurve )
{
    QwtPlotCanvas *canvas = qobject_cast<QwtPlotCanvas *>( plot->canvas() );
    canvas->installEventFilter( this );

    // We want the focus, but no focus rect. The
    // selected point will be highlighted instead.

    canvas->setFocusPolicy( Qt::StrongFocus );
#ifndef QT_NO_CURSOR
    //canvas->setCursor( Qt::PointingHandCursor );
#endif
    canvas->setFocusIndicator( QwtPlotCanvas::ItemFocusIndicator );
    canvas->setFocus();

}

Spectrogram1DPlot *Spectrogram1DPlotPicker::plot(){
    return qobject_cast<Spectrogram1DPlot *>( parent() );
}

const Spectrogram1DPlot *Spectrogram1DPlotPicker::plot() const{
    return qobject_cast<const Spectrogram1DPlot *>( parent() );
}

// Hightlight the selected point
void Spectrogram1DPlotPicker::showCursor( bool showIt )
{
    if ( !m_drawingCurve )
        return;

    QwtSymbol *symbol = const_cast<QwtSymbol *>( m_drawingCurve->symbol() );

    const QBrush brush = symbol->brush();
    if ( showIt )
        symbol->setBrush( symbol->brush().color().dark( 180 ) );

    //QwtPlotDirectPainter directPainter;
    //directPainter.drawSeries( m_drawingCurve, m_selectedPoint, m_selectedPoint );

    if ( showIt )
        symbol->setBrush( brush ); // reset brush
}

bool Spectrogram1DPlotPicker::event( QEvent *ev ){
    if ( ev->type() == QEvent::User ){
        showCursor( true );
        return true;
    }
    return QObject::event( ev );
}

void Spectrogram1DPlotPicker::draw( const QPoint &pos )
{
    //check whether there is a curve object to edit
    if ( !m_drawingCurve ){
        emit errorOccurred("Spectrogram1DPlotPicker::draw: m_drawingCurve is null.");
        return;
    }

    //get the current geometry points
    QVector<QPointF> drawnPointsSoFar;
    for( uint i = 0; i < m_drawingCurve->data()->size(); ++i){
        drawnPointsSoFar.push_back( m_drawingCurve->data()->sample( i ) );
    }

    //get the mouse position in 1D spectrogram coordinates
    double x = plot()->invTransform( m_drawingCurve->xAxis(), pos.x() );
    double y = plot()->invTransform( m_drawingCurve->yAxis(), pos.y() );

    //append the new point to the list with the current curve points
    drawnPointsSoFar.push_back( QPointF( x, y ) );

    //set curve geometry
    m_drawingCurve->setSamples( drawnPointsSoFar );

    /*
       Enable QwtPlotCanvas::ImmediatePaint, so that the canvas has been
       updated before we paint the cursor on it.
     */
    QwtPlotCanvas *plotCanvas =
        qobject_cast<QwtPlotCanvas *>( plot()->canvas() );

    plotCanvas->setPaintAttribute( QwtPlotCanvas::ImmediatePaint, true );
    plot()->replot();
    plotCanvas->setPaintAttribute( QwtPlotCanvas::ImmediatePaint, false );

    //notify any listening client code of changes in the drawing curve
    emit curveChanged( m_drawingCurve );
}


bool Spectrogram1DPlotPicker::eventFilter( QObject *object, QEvent *event ){
    if ( plot() == nullptr || object != plot()->canvas() )
        return false;

    switch( event->type() )
    {
        case QEvent::MouseButtonPress:
        {
            const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>( event );
            draw( mouseEvent->pos() );
            return true;
        }
        case QEvent::MouseMove:
        {
            const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>( event );
            draw( mouseEvent->pos() );
            return true;
        }
        default:
            break;
    }

    return QObject::eventFilter( object, event );
}
