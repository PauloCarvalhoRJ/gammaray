#ifndef IMAGEJOCKEYGRIDPLOT_H
#define IMAGEJOCKEYGRIDPLOT_H

#include <qwt_plot.h>

class QwtPlotSpectrogram;

/** Widget used in ImageJockeyDialog to display grid data. */
class ImageJockeyGridPlot: public QwtPlot
{
    Q_OBJECT

public:
    enum ColorMap
    {
        RGBMap,
        IndexMap,
        HueMap,
        AlphaMap
    };

    ImageJockeyGridPlot( QWidget * = nullptr );

public Q_SLOTS:
    void showContour( bool on );
    void showSpectrogram( bool on );
    void setColorMap( int );
    void setAlpha( int );


private:
    QwtPlotSpectrogram *m_spectrogram;

    int m_mapType;
    int m_alpha;
};

#endif // IMAGEJOCKEYGRIDPLOT_H
