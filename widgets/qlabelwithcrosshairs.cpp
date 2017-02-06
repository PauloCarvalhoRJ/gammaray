#include "qlabelwithcrosshairs.h"

#include <QMouseEvent>
#include <QPoint>
#include <QRubberBand>

QLabelWithCrossHairs::QLabelWithCrossHairs(QWidget *parent) :
    QLabel(parent),
    m_rubberBandH( nullptr ),
    m_rubberBandV( nullptr ),
    m_showCrossHairs( false )
{
    // by default, mouseMoveEvent only fires with a mouse button pressed.
    this->setMouseTracking( true );
    m_rubberBandH = new QRubberBand(QRubberBand::Line, this);
    m_rubberBandV = new QRubberBand(QRubberBand::Line, this);
}

void QLabelWithCrossHairs::mouseMoveEvent(QMouseEvent *event)
{
    m_rubberBandH->setGeometry(0, event->pos().y(),this->width(),1);
    m_rubberBandV->setGeometry(event->pos().x(), 0, 1, this->height());
}

void QLabelWithCrossHairs::toggleCrossHairs()
{
    if( ! m_showCrossHairs )
        m_showCrossHairs = true;
    else
        m_showCrossHairs = false;

    if( m_showCrossHairs ){
        m_rubberBandH->show();
        m_rubberBandV->show();
    } else {
        m_rubberBandH->hide();
        m_rubberBandV->hide();
    }
}

