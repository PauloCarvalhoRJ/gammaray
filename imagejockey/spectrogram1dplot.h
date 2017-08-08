#ifndef SPECTROGRAM1DPLOT_H
#define SPECTROGRAM1DPLOT_H

#include <qwt_plot.h>

class Attribute;
class QwtPlotCurve;

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

private:
    Attribute* m_at;

    /** The spectrogram graph curve. */
    QwtPlotCurve *m_curve;

    /** The reference value for decibel scaling. */
    double m_decibelRefValue;
};

#endif // SPECTROGRAM1DPLOT_H
