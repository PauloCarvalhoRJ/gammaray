#ifndef VERTICALPROPORTIONCURVESCANVASPICKER_H
#define VERTICALPROPORTIONCURVESCANVASPICKER_H

#include <QObject>

class QPoint;
class QCustomEvent;
class QwtPlot;
class QwtPlotCurve;
class VerticalProportionCurvesPlot;

class VerticalProportionCurvesCanvasPicker : public QObject
{
    Q_OBJECT
public:
    VerticalProportionCurvesCanvasPicker( QwtPlot *plot );
    virtual ~VerticalProportionCurvesCanvasPicker();

    virtual bool eventFilter( QObject *, QEvent * );

    virtual bool event( QEvent * );

Q_SIGNALS:
    void curveChanged( QwtPlotCurve* changed_curve );

private:
    void select( const QPoint & );
    void move( const QPoint & );
    void moveBy( int dx, int dy );

    void release();

    void showCursor( bool enable );
    void shiftPointCursor( bool up );
    void shiftCurveCursor( bool up );

    VerticalProportionCurvesPlot *plot();
    const VerticalProportionCurvesPlot *plot() const;

    QwtPlotCurve *d_selectedCurve;
    int d_selectedPoint;
};

#endif // VERTICALPROPORTIONCURVESCANVASPICKER_H
