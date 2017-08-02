#ifndef IMAGEJOCKEYGRIDPLOT_H
#define IMAGEJOCKEYGRIDPLOT_H

#include <qwt_plot.h>

class QwtPlotSpectrogram;
class Attribute;
class SpectrogramData;
class QwtPlotZoomer;

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

    /** Sets the Cartesian grid Attribute to display. */
    void setAttribute( Attribute *at );

    double getScaleMaxValue();
    double getScaleMinValue();

public Q_SLOTS:
    void showContour( bool on );
    void showSpectrogram( bool on );
    void setColorMap( int );
    void setAlpha( int );
    void setColorScaleMax( double value );
    void setColorScaleMin( double value );


private:
    QwtPlotSpectrogram *m_spectrogram;

    int m_mapType;
    int m_alpha;

    /** Attribute (of a Cartesian grid) being displayed. */
    Attribute* m_at;

    /** Adapter between QwtRasterData and Attribute of a Cartesian grid. */
    SpectrogramData* m_spectrumData;

    /** Object that controls the grid's zoom stack. */
    QwtPlotZoomer* m_zoomer;

    double m_colorScaleMax;
    double m_colorScaleMin;
};

#endif // IMAGEJOCKEYGRIDPLOT_H
