#ifndef IMAGEJOCKEYGRIDPLOT_H
#define IMAGEJOCKEYGRIDPLOT_H

#include <qwt_plot.h>

class QwtPlotSpectrogram;
class IJAbstractVariable;
class SpectrogramData;
class QwtPlotZoomer;
class QwtPlotCurve;
class SVDFactor;
class FactorData;

enum class ColorScaleForSVDFactor : int {
    LINEAR,
    LOG
};

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

    /** Sets the grid variable to display. If, set replaces the previous variable or SVDFactor.*/
    void setVariable(IJAbstractVariable *var );

	/** Sets the SVD Factor to display (it is a regular grid). If, set replaces the previous Attribute or SVDFactor.*/
	void setSVDFactor( SVDFactor* svdFactor);

    /** Sets the color scaling (linear or log) for the SVD factor being viewed.
     * This method has no effect if no SVD factor is being viewed or there is a Cartesian grid being viewed.
     */
    void setColorScaleForSVDFactor( ColorScaleForSVDFactor setting );

    //@{
    /** Getters for the min/max values of the color scale (Z-axis). */
    double getScaleMaxValue();
    double getScaleMinValue();
    //@}

    /** Sets the zoom to cover the entire grid. */
    void pan();

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

    /** Variable (of a regular grid) being displayed. */
    IJAbstractVariable* m_var;

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
