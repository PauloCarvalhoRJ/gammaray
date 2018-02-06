#ifndef SPECTROGRAM1DPLOT_H
#define SPECTROGRAM1DPLOT_H

#include <qwt_plot.h>

class IJAbstractVariable;
class QwtPlotCurve;

/** Widget used in ImageJockeyDialog to display a 1D spectrogram along a band in a 2D Fourier image
 *  The grid values are displayed as their absolute values in decibel scaling for ease of
 *  interpretation since the frequency component intensities show great variation in order of magnitude.
 */

class Spectrogram1DPlot : public QwtPlot
{

    Q_OBJECT

public:
    Spectrogram1DPlot(QWidget * parent = nullptr);
    virtual bool eventFilter(QObject *object, QEvent * e);

public Q_SLOTS:
    void setVariable(IJAbstractVariable* var );
    /** Called when it is necessary to retrieve spectrography data from the Attribute's grid. */
    void rereadSpectrogramData();
    void setDecibelRefValue( double value );
    void setVerticalScaleMax( double value );
    void setVerticalScaleMin( double value );
    void setHorizontalScaleMax( double value ); //minimum is always zero (DC == 0 frquency).
    void updateFrequencyWindow( double begin, double end );
    void resetReferenceCurve();

private:
    IJAbstractVariable* m_var;

    /** The spectrogram graph curve. */
    QwtPlotCurve *m_curve;

    /** The reference value for decibel scaling. */
    double m_decibelRefValue;

    double m_yScaleMax;
    double m_yScaleMin;
    double m_xScaleMax; //minimum frequency is always zero (DC) for Fourier images computed from real-only values.

    //@{
    /** The two vertical lines representing the current frequency window for the graphic equalizer. */
    QwtPlotCurve *m_frequencyWindowBeginCurve;
    QwtPlotCurve *m_frequencyWindowEndCurve;
    double m_freqWindowBegin;
    double m_freqWindowEnd;
    void updateFrequencyWindowLines();
    //@}

    /** A user-drawn curve so one can visually compare several 1D spectra against a reference spectrum. */
    QwtPlotCurve *m_referenceCurve;

private Q_SLOTS:
    void onReferenceCurveChanged( QwtPlotCurve* refCurve );
};

#endif // SPECTROGRAM1DPLOT_H
