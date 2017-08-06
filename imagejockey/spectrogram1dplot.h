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

private:
    Attribute* m_at;

    /** The spectrogram graph curve. */
    QwtPlotCurve *m_curve;
};

#endif // SPECTROGRAM1DPLOT_H
