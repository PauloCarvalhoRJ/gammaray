#ifndef SPECTROGRAM1DPLOTPICKER_H
#define SPECTROGRAM1DPLOTPICKER_H

#include <qobject.h>

class QwtPlot;
class Spectrogram1DPlot;
class QwtPlotCurve;

/** The object that handles the user input events on the 1D spectrogram canvas and acts on the various objects
 * composing the 1D spectrogram.
 */
class Spectrogram1DPlotPicker : public QObject
{
        Q_OBJECT

public:
    Spectrogram1DPlotPicker( QwtPlotCurve* drawingCurve, QwtPlot *plot );
    virtual bool eventFilter( QObject *, QEvent * );
    virtual bool event( QEvent * );

Q_SIGNALS:
    void curveChanged( QwtPlotCurve* changed_curve );
    void errorOccurred( QString message );

private:

    //@{
    /** Boilerplate code. */
    Spectrogram1DPlot *plot();
    const Spectrogram1DPlot *plot() const;
    //@}

    /** Called to change the mouse pointer appearance when the cursor enters or leaves the plot area. */
    void showCursor( bool enable );

    /** Called while the user drags the mouse through the plot area to draw a line. */
    void draw( const QPoint & );

    /** The pointer to the drawing curve object to receive the user-drawn points. */
    QwtPlotCurve *m_drawingCurve;
};

#endif // SPECTROGRAM1DPLOTPICKER_H
