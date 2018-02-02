#ifndef IMAGEJOCKEYGRIDPLOT_H
#define IMAGEJOCKEYGRIDPLOT_H

#include <qwt_plot.h>

class QwtPlotSpectrogram;
class Attribute;
class SpectrogramData;
class QwtPlotZoomer;
class QwtPlotCurve;
class SVDFactor;
class FactorData;

/** Widget used in ImageJockeyDialog to display a grid containing a Fourier image
 *  The grid values are displayed as their absolute values in decibel scaling for ease of
 *  interpretation since the frequency component intensities show great variation in order of magnitude.
 */
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

	/** Sets the Cartesian grid Attribute to display. If, set replaces the previous Attribute or SVDFactor.*/
    void setAttribute( Attribute *at );

	/** Sets the SVD Factor to display (it is a regular grid). If, set replaces the previous Attribute or SVDFactor.*/
	void setSVDFactor( SVDFactor* svdFactor);

    double getScaleMaxValue();
    double getScaleMinValue();

public Q_SLOTS:
    void showContour( bool on );
    void showSpectrogram( bool on );
    void setColorMap( int );
    void setAlpha( int );
    void setColorScaleMax( double value );
    void setColorScaleMin( double value );
    void setDecibelRefValue( double value );
    void draw1DSpectrogramBand();

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

    /** End-of-scale value. */
    double m_colorScaleMax;

    /** Begin-of-scale value. */
    double m_colorScaleMin;

	/** SVD factor (a type of regular grid) being displayed. */
	SVDFactor* m_svdFactor;

	/** Adapter between QwtRasterData and an SVDFactor (a type of regular grid). */
	FactorData* m_factorData;

    QwtPlotCurve *m_curve1DSpectrogramHalfBand1;
    QwtPlotCurve *m_curve1DSpectrogramHalfBand2;
};

#endif // IMAGEJOCKEYGRIDPLOT_H
