#ifndef SPECTROGRAM1DPLOTPICKER_H
#define SPECTROGRAM1DPLOTPICKER_H

#include <qobject.h>

class QwtPlot;
class Spectrogram1DPlot;
class QwtPlotCurve;

/** The object that handles the input events and acts on the various objects
 * composing the 1D spectrogram.
 *
 * TODO: this class is currently not being used.  Kept for possible future application.
 */
class Spectrogram1DPlotPicker : public QObject
{
        Q_OBJECT

public:
    Spectrogram1DPlotPicker(  QwtPlot *plot );
    virtual bool eventFilter( QObject *, QEvent * );
    virtual bool event( QEvent * );

Q_SIGNALS:
    void curveChanged( QwtPlotCurve* changed_curve );

private:
    Spectrogram1DPlot *plot();
    const Spectrogram1DPlot *plot() const;
    void showCursor( bool enable );
    void select( const QPoint & );
    void move( const QPoint & );

    void moveBy( int dx, int dy );
    void shiftPointCursor( bool up );
    void shiftCurveCursor( bool up );

    QwtPlotCurve *m_selectedCurve;
    int m_selectedPoint;

};

#endif // SPECTROGRAM1DPLOTPICKER_H
