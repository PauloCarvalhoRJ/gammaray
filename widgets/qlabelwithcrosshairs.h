#ifndef QLABELWITHCROSSHAIRS_H
#define QLABELWITHCROSSHAIRS_H

#include <QLabel>

class QRubberBand;

/**
 * The QLabelWithCrossHairs extends Qt's QLabel with an option to show crosshairs
 * under the mouse pointer.  This is useful to analyse any charts that the user may
 * plot inside this widget.
 */
class QLabelWithCrossHairs : public QLabel
{
    Q_OBJECT
public:
    explicit QLabelWithCrossHairs(QWidget *parent = 0);

    void mouseMoveEvent(QMouseEvent *event);

    /**
     * Toggles the display of a cross hairs under the mouse pointer.
     */
    void toggleCrossHairs( );

signals:

public slots:

private:
    QPoint m_origin;
    QRubberBand* m_rubberBandH;
    QRubberBand* m_rubberBandV;
    bool m_showCrossHairs;
};

#endif // QLABELWITHCROSSHAIRS_H
