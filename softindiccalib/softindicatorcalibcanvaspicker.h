#include <qobject.h>

class QPoint;
class QCustomEvent;
class QwtPlot;
class QwtPlotCurve;
class SoftIndicatorCalibPlot;

class SoftIndicatorCalibCanvasPicker: public QObject
{
    Q_OBJECT
public:
    SoftIndicatorCalibCanvasPicker( QwtPlot *plot );
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

    SoftIndicatorCalibPlot *plot();
    const SoftIndicatorCalibPlot *plot() const;

    QwtPlotCurve *d_selectedCurve;
    int d_selectedPoint;
};
