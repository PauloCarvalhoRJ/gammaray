#include "spectrogram1dplotpicker.h"

#include "spectrogram1dplot.h"
#include "domain/application.h"

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

// Select the point at a position. If there is no point
// deselect the selected point
void Spectrogram1DPlotPicker::beginDraw( const QPoint &pos ){
    QwtPlotCurve *curve = nullptr;
    double dist = 10e10;
    int index = -1;

    const QwtPlotItemList& itmList = plot()->itemList();
    for ( QwtPlotItemIterator it = itmList.begin();
        it != itmList.end(); ++it )
    {
        if ( ( *it )->rtti() == QwtPlotItem::Rtti_PlotCurve )
        {
            QwtPlotCurve *c = static_cast<QwtPlotCurve *>( *it );

            double d;
            int idx = c->closestPoint( pos, &d );
            if ( d < dist )
            {
                curve = c;
                index = idx;
                dist = d;
            }
        }
    }

    showCursor( false );

    if ( curve && dist < 10 ) // 10 pixels tolerance
    {
        m_drawingCurve = curve;
        showCursor( true );
    }
}

// Move the selected point
void Spectrogram1DPlotPicker::draw( const QPoint &pos )
{
    if ( !m_drawingCurve )
        return;

    Application::instance()->logError("LALALAL");

    QVector<QPointF> drawnPointsSoFar;
    for( int i = 0; i < m_drawingCurve->data()->size(); ++i){
        drawnPointsSoFar.push_back( m_drawingCurve->data()->sample( i ) );
    }

    //get the mouse position in 1D spectrogram coordinates
    double x = plot()->invTransform( m_drawingCurve->xAxis(), pos.x() );
    double y = plot()->invTransform( m_drawingCurve->yAxis(), pos.y() );

    drawnPointsSoFar.push_back( QPointF( x, y ) );

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

    showCursor( true );

    //notify any listening client code of changes in the drawing curve
    emit curveChanged( m_drawingCurve );
}

// Select the next/previous curve
void Spectrogram1DPlotPicker::shiftCurveCursor( bool up )
{
    QwtPlotItemIterator it;

    const QwtPlotItemList &itemList = plot()->itemList();

    QwtPlotItemList curveList;
    for ( it = itemList.begin(); it != itemList.end(); ++it )
    {
        if ( ( *it )->rtti() == QwtPlotItem::Rtti_PlotCurve )
            curveList += *it;
    }
    if ( curveList.isEmpty() )
        return;

    it = curveList.begin();

    if ( m_drawingCurve )
    {
        for ( it = curveList.begin(); it != curveList.end(); ++it )
        {
            if ( m_drawingCurve == *it )
                break;
        }
        if ( it == curveList.end() ) // not found
            it = curveList.begin();

        if ( up )
        {
            ++it;
            if ( it == curveList.end() )
                it = curveList.begin();
        }
        else
        {
            if ( it == curveList.begin() )
                it = curveList.end();
            --it;
        }
    }

    showCursor( false );
    m_drawingCurve = static_cast<QwtPlotCurve *>( *it );
    showCursor( true );
}

// Select the next/previous neighbour of the selected point
void Spectrogram1DPlotPicker::shiftPointCursor( bool up )
{
    if ( !m_drawingCurve )
        return;
}

bool Spectrogram1DPlotPicker::eventFilter( QObject *object, QEvent *event ){
    Application::instance()->logError("LILILILI");
    if ( plot() == nullptr || object != plot()->canvas() )
        return false;

    Application::instance()->logError("LOLOLOLO");

    switch( event->type() )
    {
        case QEvent::FocusIn:
        {
            showCursor( true );
            break;
        }
        case QEvent::FocusOut:
        {
            showCursor( false );
            break;
        }
        case QEvent::Paint:
        {
            //QApplication::postEvent( this, new QEvent( QEvent::User ) );
            break;
        }
        case QEvent::MouseButtonPress:
        {
            const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>( event );
            beginDraw( mouseEvent->pos() );
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
