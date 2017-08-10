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

Spectrogram1DPlotPicker::Spectrogram1DPlotPicker(QwtPlot *plot) :
    QObject( plot ),
    m_selectedCurve( nullptr )
{
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
    if ( !m_selectedCurve )
        return;

    QwtSymbol *symbol = const_cast<QwtSymbol *>( m_selectedCurve->symbol() );

    const QBrush brush = symbol->brush();
    if ( showIt )
        symbol->setBrush( symbol->brush().color().dark( 180 ) );

    QwtPlotDirectPainter directPainter;
    directPainter.drawSeries( m_selectedCurve, m_selectedPoint, m_selectedPoint );

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
void Spectrogram1DPlotPicker::select( const QPoint &pos ){
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
    m_selectedCurve = nullptr;
    m_selectedPoint = -1;

    if ( curve && dist < 10 ) // 10 pixels tolerance
    {
        m_selectedCurve = curve;
        m_selectedPoint = index;
        showCursor( true );
    }
}

// Move the selected point
void Spectrogram1DPlotPicker::move( const QPoint &pos )
{
    if ( !m_selectedCurve )
        return;

    QVector<double> xData( m_selectedCurve->dataSize() );
    QVector<double> yData( m_selectedCurve->dataSize() );

    for ( int i = 0;
        i < static_cast<int>( m_selectedCurve->dataSize() ); i++ )
    {
        if ( i == m_selectedPoint )
        {
            //move the curve points only up and down
            xData[i] = m_selectedCurve->sample(i).x();
            //xData[i] = plot()->invTransform(
                //d_selectedCurve->xAxis(), pos.x() );
            yData[i] = plot()->invTransform(
                m_selectedCurve->yAxis(), pos.y() );
        }
        else
        {
            const QPointF sample = m_selectedCurve->sample( i );
            xData[i] = sample.x();
            yData[i] = sample.y();
        }
        //limits vertical movement to a range between 0.0 and 100.0,
        //since there is no valid probabilty value outside that range
        if( yData[i] > 100.0)
            yData[i] = 100.0;
        if( yData[i] < 0.0)
            yData[i] = 0.0;
    }


    m_selectedCurve->setSamples( xData, yData );

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

    //notify any listening client code of changes in the curves
    emit curveChanged( m_selectedCurve );
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

    if ( m_selectedCurve )
    {
        for ( it = curveList.begin(); it != curveList.end(); ++it )
        {
            if ( m_selectedCurve == *it )
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
    m_selectedPoint = 0;
    m_selectedCurve = static_cast<QwtPlotCurve *>( *it );
    showCursor( true );
}

// Select the next/previous neighbour of the selected point
void Spectrogram1DPlotPicker::shiftPointCursor( bool up )
{
    if ( !m_selectedCurve )
        return;

    int index = m_selectedPoint + ( up ? 1 : -1 );
    index = ( index + m_selectedCurve->dataSize() ) % m_selectedCurve->dataSize();

    if ( index != m_selectedPoint )
    {
        showCursor( false );
        m_selectedPoint = index;
        showCursor( true );
    }
}

// Move the selected point
void Spectrogram1DPlotPicker::moveBy( int dx, int dy )
{
    if ( dx == 0 && dy == 0 )
        return;

    if ( !m_selectedCurve )
        return;

    const QPointF sample =
        m_selectedCurve->sample( m_selectedPoint );

    const double x = plot()->transform(
        m_selectedCurve->xAxis(), sample.x() );
    const double y = plot()->transform(
        m_selectedCurve->yAxis(), sample.y() );

    move( QPoint( qRound( x + dx ), qRound( y + dy ) ) );
}

bool Spectrogram1DPlotPicker::eventFilter( QObject *object, QEvent *event ){
    if ( plot() == nullptr || object != plot()->canvas() )
        return false;

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
            QApplication::postEvent( this, new QEvent( QEvent::User ) );
            break;
        }
        case QEvent::MouseButtonPress:
        {
            const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>( event );
            select( mouseEvent->pos() );
            return true;
        }
        case QEvent::MouseMove:
        {
            const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>( event );
            move( mouseEvent->pos() );
            return true;
        }
        case QEvent::KeyPress:
        {
            const QKeyEvent *keyEvent = static_cast<QKeyEvent *>( event );

            const int delta = 5;
            switch( keyEvent->key() )
            {
                case Qt::Key_Up:
                {
                    shiftCurveCursor( true );
                    return true;
                }
                case Qt::Key_Down:
                {
                    shiftCurveCursor( false );
                    return true;
                }
                case Qt::Key_Right:
                case Qt::Key_Plus:
                {
                    if ( m_selectedCurve )
                        shiftPointCursor( true );
                    else
                        shiftCurveCursor( true );
                    return true;
                }
                case Qt::Key_Left:
                case Qt::Key_Minus:
                {
                    if ( m_selectedCurve )
                        shiftPointCursor( false );
                    else
                        shiftCurveCursor( true );
                    return true;
                }

                // The following keys represent a direction, they are
                // organized on the keyboard.

                case Qt::Key_1:
                {
                    moveBy( -delta, delta );
                    break;
                }
                case Qt::Key_2:
                {
                    moveBy( 0, delta );
                    break;
                }
                case Qt::Key_3:
                {
                    moveBy( delta, delta );
                    break;
                }
                case Qt::Key_4:
                {
                    moveBy( -delta, 0 );
                    break;
                }
                case Qt::Key_6:
                {
                    moveBy( delta, 0 );
                    break;
                }
                case Qt::Key_7:
                {
                    moveBy( -delta, -delta );
                    break;
                }
                case Qt::Key_8:
                {
                    moveBy( 0, -delta );
                    break;
                }
                case Qt::Key_9:
                {
                    moveBy( delta, -delta );
                    break;
                }
                default:
                    break;
            }
        }
        default:
            break;
    }

    return QObject::eventFilter( object, event );
}
