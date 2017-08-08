#ifndef SPECTROGRAM1DPLOT_H
#define SPECTROGRAM1DPLOT_H

#include <qwt_plot.h>

class Attribute;
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

public Q_SLOTS:
    void setAttribute( Attribute* at );
    /** Called when it is necessary to retrieve spectrography data from the Attribute's grid. */
    void rereadSpectrogramData();
    void setDecibelRefValue( double value );
    void setVerticalScaleMax( double value );
    void setVerticalScaleMin( double value );
    void setHorizontalScaleMax( double value ); //minimum is always zero (DC == 0 frquency).

private:
    Attribute* m_at;

    /** The spectrogram graph curve. */
    QwtPlotCurve *m_curve;

    /** The reference value for decibel scaling. */
    double m_decibelRefValue;

    double m_yScaleMax;
    double m_yScaleMin;
    double m_xScaleMax;
};

#endif // SPECTROGRAM1DPLOT_H
