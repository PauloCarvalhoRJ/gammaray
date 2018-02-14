#include "imagejockeygridplot.h"
#include <qwt_plot_spectrogram.h>
#include <qnumeric.h>
#include <qwt_color_map.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_scale_widget.h>
#include <qwt_scale_draw.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_renderer.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>

#include <qapplication.h>

#include "ijabstractvariable.h"
#include "ijabstractcartesiangrid.h"
#include "spectrogram1dparameters.h"
#include "ijmatrix3x3.h"
#include "imagejockeyutils.h"
#include "svd/svdfactor.h"

//////////////////////////////////////////////ZOOMER CLASS////////////////////////////
class SpectrogramZoomer: public QwtPlotZoomer
{
public:
    SpectrogramZoomer( QWidget *canvas ):  QwtPlotZoomer( canvas ) {
        setTrackerMode( AlwaysOn );
    }
    /** Returns the text to be displayed next to the mouse pointer while it hovers
     * over the grid plot area. */
    virtual QwtText trackerTextF( const QPointF &pos ) const  {
        QColor bg( Qt::white );
        bg.setAlpha( 200 );

        QwtText text = QwtPlotZoomer::trackerTextF( pos );
        text.setBackgroundBrush( QBrush( bg ) );
        return text;
    }
};

///////////////////////////////////////////RASTER DATA ADAPTER: QwtRasterData <-> CartesianGrid ///////////////////////
class SpectrogramData: public QwtRasterData{
public:
    SpectrogramData() : m_var( nullptr ), m_cg( nullptr ), m_decibelRefValue(100.0) {
        //set some default values before the user chooses a grid
        setInterval( Qt::XAxis, QwtInterval( -1.5, 1.5 ) );
        setInterval( Qt::YAxis, QwtInterval( -1.5, 1.5 ) );
        setInterval( Qt::ZAxis, QwtInterval( 0.0, 10.0 ) );
    }
    /** Returns a grid value as a function of spatial location.
     *  Returns NaN if the location is outside the grid or corresponds to an unvalued cell.
     */
    virtual double value( double x, double y ) const {
        if( ! m_var ) //no grid selected
            //returning a NaN means a blank plot
            return std::numeric_limits<double>::quiet_NaN();
        else{
            // get the grid value as is
            double value = m_cg->getDataAt( m_var->getIndexInParentGrid(), x, y, 0);
            if( m_cg->isNoDataValue(value) ) //if there is no value there
                value = std::numeric_limits<double>::quiet_NaN(); //returns NaN (blank plot)
            else
                //for Fourier images, get the absolute values in decibel for ease of interpretation
                value = ImageJockeyUtils::dB( std::abs<double>(value), m_decibelRefValue, 0.0000001 );
            return value;
        }
    }
    /** Define the Attribute of a Cartesian grid.  Resets the plot. */
    void setAttribute( IJAbstractVariable* var ){
        m_var = var;
        if( var ){
            //get the variables's parent grid
            m_cg = var->getParentGrid();
            //get the column number in the parent grid
            int columnIndex = var->getIndexInParentGrid();
            //resets the map display coverage to show the entire map
            setInterval( Qt::XAxis, QwtInterval( m_cg->getOriginX() - m_cg->getCellSizeI(),
                                                 m_cg->getOriginX() + m_cg->getCellSizeI() * m_cg->getNI() ) );
            setInterval( Qt::YAxis, QwtInterval( m_cg->getOriginY() - m_cg->getCellSizeJ(),
                                                 m_cg->getOriginY() + m_cg->getCellSizeJ() * m_cg->getNJ() ) );
			//preload data from file, prefetch data from database...
			m_cg->dataWillBeRequested();
            //for Fourier images, get the absolute values in decibel for ease of interpretation
            double min = ImageJockeyUtils::dB( m_cg->absMin( columnIndex ), m_decibelRefValue, 0.0000001 );
            double max = ImageJockeyUtils::dB( m_cg->absMax( columnIndex ), m_decibelRefValue, 0.0000001 );
            //Z in a 2D raster plot is the attribute value, not the Z coordinate.
            setInterval( Qt::ZAxis, QwtInterval( min, max ));
        }
        else
            m_cg = nullptr; //resets Cartesian grid pointer if variable is null
    }
    /** Defines the reference value for decibel scaling. */
    void setDecibelRefValue( double value ){
        m_decibelRefValue = value;
    }
private:
    /** The variable being displayed. */
    IJAbstractVariable* m_var;
    /** The variables's Cartesian grid. */
    IJAbstractCartesianGrid* m_cg;
    /** The reference value for decibel scaling. */
    double m_decibelRefValue;
};

///////////////////////////////////////////RASTER DATA ADAPTER: QwtRasterData <-> SVDFactor ///////////////////////
class FactorData: public QwtRasterData{
public:
    FactorData() : m_factor( nullptr ), m_colorScaleForSVDFactor( ColorScaleForSVDFactor::LINEAR ) {
		//set some default values before the user chooses a factor
		setInterval( Qt::XAxis, QwtInterval( -1.5, 1.5 ) );
		setInterval( Qt::YAxis, QwtInterval( -1.5, 1.5 ) );
		setInterval( Qt::ZAxis, QwtInterval( 0.0, 10.0 ) );
	}
	/** Returns a grid value as a function of spatial location.
	 *  Returns NaN if the location is outside the grid or corresponds to an unvalued cell.
	 */
	virtual double value( double x, double y ) const {
		if( ! m_factor ) //no factor selected
			//returning a NaN means a blank plot
			return std::numeric_limits<double>::quiet_NaN();
		else{
			// get the grid value as is
			double value = m_factor->valueAtCurrentPlane( x, y );
			if( m_factor->isNDV(value) ) //if there is no value there
				value = std::numeric_limits<double>::quiet_NaN(); //returns NaN (blank plot)
            if( m_colorScaleForSVDFactor == ColorScaleForSVDFactor::LOG )
				value = std::log10( std::abs( value ) );
			return value;
		}
	}
	/** Define the factor.  Resets the plot. */
	void setFactor( SVDFactor* factor ){
		m_factor = factor;
		if( factor ){
			//resets the map display coverage to show the entire map
			setInterval( Qt::XAxis, QwtInterval( m_factor->getCurrentPlaneX0() - m_factor->getCurrentPlaneDX(),
												 m_factor->getCurrentPlaneX0() + m_factor->getCurrentPlaneDX() * m_factor->getCurrentPlaneNX() ) );
			setInterval( Qt::YAxis, QwtInterval( m_factor->getCurrentPlaneY0() - m_factor->getCurrentPlaneDY(),
												 m_factor->getCurrentPlaneY0() + m_factor->getCurrentPlaneDY() * m_factor->getCurrentPlaneNY() ) );
			//for Fourier images, get the absolute values in decibel for ease of interpretation
			double min = m_factor->getMinValue();
			double max = m_factor->getMaxValue();
			//Z in a 2D raster plot is the attribute value, not the Z coordinate.
			setInterval( Qt::ZAxis, QwtInterval( min, max ));
		}
	}
    /** Set the color scale for viewing the SVD factor. */
    void setColorScale( ColorScaleForSVDFactor setting ){
        m_colorScaleForSVDFactor = setting;
    }

private:
	/** The SVD Factor being displayed. */
	SVDFactor* m_factor;
    ColorScaleForSVDFactor m_colorScaleForSVDFactor;
};

/////////////////////////////////////////////A COLOR MAP/////////////////////////////
class LinearColorMapRGB: public QwtLinearColorMap
{
public:
    LinearColorMapRGB():
        QwtLinearColorMap( Qt::darkBlue, Qt::red, QwtColorMap::RGB )
    {
        addColorStop( 0.25, Qt::cyan );
        addColorStop( 0.5, Qt::green );
        addColorStop( 0.75, Qt::yellow );
    }
};

/////////////////////////////////////////////A COLOR MAP/////////////////////////////
class LinearColorMapIndexed: public QwtLinearColorMap
{
public:
    LinearColorMapIndexed():
        QwtLinearColorMap( Qt::darkCyan, Qt::red, QwtColorMap::Indexed )
    {
        addColorStop( 0.1, Qt::cyan );
        addColorStop( 0.6, Qt::green );
        addColorStop( 0.95, Qt::yellow );
    }
};

/////////////////////////////////////////////A COLOR MAP/////////////////////////////
class HueColorMap: public QwtColorMap
{
public:
    // class backported from Qwt 6.2

    HueColorMap():
        d_hue1(0),
        d_hue2(359),
        d_saturation(150),
        d_value(200)
    {
        updateTable();

    }

    virtual QRgb rgb( const QwtInterval &interval, double value ) const
    {
        if ( qIsNaN(value) )
            return 0u;

        const double width = interval.width();
        if ( width <= 0 )
            return 0u;

        if ( value <= interval.minValue() )
            return d_rgbMin;

        if ( value >= interval.maxValue() )
            return d_rgbMax;

        const double ratio = ( value - interval.minValue() ) / width;
        int hue = d_hue1 + qRound( ratio * ( d_hue2 - d_hue1 ) );

        if ( hue >= 360 )
        {
            hue -= 360;

            if ( hue >= 360 )
                hue = hue % 360;
        }

        return d_rgbTable[hue];
    }

    virtual unsigned char colorIndex( const QwtInterval &, double ) const
    {
        // we don't support indexed colors
        return 0;
    }


private:
    void updateTable()
    {
        for ( int i = 0; i < 360; i++ )
            d_rgbTable[i] = QColor::fromHsv( i, d_saturation, d_value ).rgb();

        d_rgbMin = d_rgbTable[ d_hue1 % 360 ];
        d_rgbMax = d_rgbTable[ d_hue2 % 360 ];
    }

    int d_hue1, d_hue2, d_saturation, d_value;
    QRgb d_rgbMin, d_rgbMax, d_rgbTable[360];
};

/////////////////////////////////////////////A COLOR MAP/////////////////////////////
class AlphaColorMap: public QwtAlphaColorMap
{
public:
    AlphaColorMap()
    {
        //setColor( QColor("DarkSalmon") );
        setColor( QColor("SteelBlue") );
    }
};

/////////////////////////////////////////////THE IMAGE JOCKEY PLOT CLASS ITSELF/////////////////////////////
ImageJockeyGridPlot::ImageJockeyGridPlot( QWidget *parent ):
    QwtPlot( parent ),
    m_alpha(255),
    m_var( nullptr ),
    m_zoomer( nullptr ),
    m_colorScaleMax( 10.0 ),
    m_colorScaleMin( 0.0 ),
	m_svdFactor( nullptr ),
    m_curve1DSpectrogramHalfBand1( nullptr ),
    m_curve1DSpectrogramHalfBand2( nullptr )
{
    m_spectrogram = new QwtPlotSpectrogram();
    m_spectrogram->setRenderThreadCount( 0 ); // use system specific thread count
    m_spectrogram->setCachePolicy( QwtPlotRasterItem::PaintCache );

    QList<double> contourLevels;
    for ( double level = 0.5; level < 10.0; level += 1.0 )
        contourLevels += level;
    m_spectrogram->setContourLevels( contourLevels );

	m_factorData = new FactorData(); //not used initially

    m_spectrumData = new SpectrogramData();
    m_spectrogram->setData( m_spectrumData );
    m_spectrogram->attach( this );

    const QwtInterval zInterval = m_spectrogram->data()->interval( Qt::ZAxis );

    // A color bar on the right axis
    QwtScaleWidget *rightAxis = axisWidget( QwtPlot::yRight );
    rightAxis->setTitle( "" );
    rightAxis->setColorBarEnabled( true );

    setAxisScale( QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue() );
    enableAxis( QwtPlot::yRight );

    plotLayout()->setAlignCanvasToScales( true );

    setColorMap( ImageJockeyGridPlot::RGBMap );

    // LeftButton for the zooming
    // MidButton for the panning
    // RightButton: zoom out by 1
    // Ctrl+RighButton: zoom out to full size

    m_zoomer = new SpectrogramZoomer( canvas() );
    m_zoomer->setMousePattern( QwtEventPattern::MouseSelect2,
        Qt::RightButton, Qt::ControlModifier );
    m_zoomer->setMousePattern( QwtEventPattern::MouseSelect3,
        Qt::RightButton );

    QwtPlotPanner *panner = new QwtPlotPanner( canvas() );
    panner->setAxisEnabled( QwtPlot::yRight, false );
    panner->setMouseButton( Qt::MidButton );

    // Avoid jumping when labels with more/less digits
    // appear/disappear when scrolling vertically

    const QFontMetrics fm( axisWidget( QwtPlot::yLeft )->font() );
    QwtScaleDraw *sd = axisScaleDraw( QwtPlot::yLeft );
    sd->setMinimumExtent( fm.width( "100.00" ) );

    //the visual presentation of the band to compute the 1D spectrogram
    const QColor c( Qt::darkBlue );
    m_zoomer->setRubberBandPen( c );
    m_zoomer->setTrackerPen( c );
    m_curve1DSpectrogramHalfBand1 = new QwtPlotCurve();
    m_curve1DSpectrogramHalfBand1->attach( this );
    m_curve1DSpectrogramHalfBand1->setPen( QPen( QColor( Qt::black ) ) );
    m_curve1DSpectrogramHalfBand2 = new QwtPlotCurve();
    m_curve1DSpectrogramHalfBand2->attach( this );
    m_curve1DSpectrogramHalfBand2->setPen( QPen( QColor( Qt::black ) ) );
}

void ImageJockeyGridPlot::setVariable(IJAbstractVariable *var)
{
    //get the data
    m_var = var;
	IJAbstractCartesianGrid *file = var->getParentGrid();
	if( ! file ){
		emit errorOccurred("ImageJockeyGridPlot::setAttribute(): Only variables of Cartesian grids are accepted.");
        m_spectrumData->setAttribute( nullptr );
    } else {
        m_spectrumData->setAttribute( var );
    }

	m_spectrogram->setData( m_spectrumData );

    //redefine color scale/legend
    const QwtInterval zInterval = m_spectrogram->data()->interval( Qt::ZAxis );
    // A color bar on the right axis
    QwtScaleWidget *rightAxis = axisWidget( QwtPlot::yRight );
	rightAxis->setTitle( var->getVariableName() + " (dB)" );
    setAxisScale( QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue() );
    setColorMap( ImageJockeyGridPlot::RGBMap );

    //set zoom to cover the entire grid
    pan();

    //reset the zoom stack to the possibly new geographic region.
    m_zoomer->setZoomBase();

	replot();
}

void ImageJockeyGridPlot::setSVDFactor(SVDFactor * svdFactor)
{
	//get the data
	m_factorData->setFactor( svdFactor );

	m_spectrogram->setData( m_factorData );

	//redefine color scale/legend
	const QwtInterval zInterval = m_spectrogram->data()->interval( Qt::ZAxis );
	// A color bar on the right axis
	QwtScaleWidget *rightAxis = axisWidget( QwtPlot::yRight );
	rightAxis->setTitle( svdFactor->getPresentationName() + " (unitless)" );
	setAxisScale( QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue() );
	setColorMap( ImageJockeyGridPlot::RGBMap );

	//set zoom to cover the entire grid
    pan();

	//reset the zoom stack to the possibly new geographic region.
	m_zoomer->setZoomBase();

    replot();
}

void ImageJockeyGridPlot::setColorScaleForSVDFactor(ColorScaleForSVDFactor setting)
{
    m_factorData->setColorScale( setting );
    replot();
    repaint();
}

double ImageJockeyGridPlot::getScaleMaxValue()
{
    return m_spectrogram->data()->interval( Qt::ZAxis ).maxValue();
}

double ImageJockeyGridPlot::getScaleMinValue()
{
    return m_spectrogram->data()->interval( Qt::ZAxis ).minValue();
}

void ImageJockeyGridPlot::pan()
{
    const QwtInterval xInterval = m_spectrogram->data()->interval( Qt::XAxis );
    setAxisScale( QwtPlot::xBottom, xInterval.minValue(), xInterval.maxValue() );
    const QwtInterval yInterval = m_spectrogram->data()->interval( Qt::YAxis );
    setAxisScale( QwtPlot::yLeft, yInterval.minValue(), yInterval.maxValue() );
}

void ImageJockeyGridPlot::forceUpdate()
{
    //this causes a redraw
    setColorScaleMax( getScaleMaxValue() );
}

void ImageJockeyGridPlot::showContour( bool on )
{
    m_spectrogram->setDisplayMode( QwtPlotSpectrogram::ContourMode, on );
    replot();
}

void ImageJockeyGridPlot::showSpectrogram( bool on )
{
    m_spectrogram->setDisplayMode( QwtPlotSpectrogram::ImageMode, on );
    m_spectrogram->setDefaultContourPen(
        on ? QPen( Qt::black, 0 ) : QPen( Qt::NoPen ) );

    replot();
}

void ImageJockeyGridPlot::setColorMap( int type )
{
    QwtScaleWidget *axis = axisWidget( QwtPlot::yRight );
    //const QwtInterval zInterval = m_spectrogram->data()->interval( Qt::ZAxis );
    const QwtInterval zInterval( m_colorScaleMin, m_colorScaleMax );

    m_mapType = type;

    int alpha = m_alpha;
    switch( type )
    {
        case ImageJockeyGridPlot::HueMap:
        {
            m_spectrogram->setColorMap( new HueColorMap() );
            axis->setColorMap( zInterval, new HueColorMap() );
            break;
        }
        case ImageJockeyGridPlot::AlphaMap:
        {
            alpha = 255;
            m_spectrogram->setColorMap( new AlphaColorMap() );
            axis->setColorMap( zInterval, new AlphaColorMap() );
            break;
        }
        case ImageJockeyGridPlot::IndexMap:
        {
            m_spectrogram->setColorMap( new LinearColorMapIndexed() );
            axis->setColorMap( zInterval, new LinearColorMapIndexed() );
            break;
        }
        case ImageJockeyGridPlot::RGBMap:
        default:
        {
            m_spectrogram->setColorMap( new LinearColorMapRGB() );
            axis->setColorMap( zInterval, new LinearColorMapRGB() );
        }
    }
    m_spectrogram->setAlpha( alpha );

    replot();
}

void ImageJockeyGridPlot::setAlpha( int alpha )
{
    // setting an alpha value doesn't make sense in combination
    // with a color map interpolating the alpha value

    m_alpha = alpha;
    if ( m_mapType != ImageJockeyGridPlot::AlphaMap )
    {
        m_spectrogram->setAlpha( alpha );
        replot();
    }
}

void ImageJockeyGridPlot::setColorScaleMax(double value)
{
    m_colorScaleMax = value;
    setColorMap( ImageJockeyGridPlot::RGBMap );
	if( m_var )
		m_spectrumData->setInterval( Qt::ZAxis, QwtInterval( m_colorScaleMin, m_colorScaleMax ) );
	m_factorData->setInterval( Qt::ZAxis, QwtInterval( m_colorScaleMin, m_colorScaleMax ) );

    const QwtInterval zInterval = m_spectrogram->data()->interval( Qt::ZAxis );
    setAxisScale( QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue() );

    replot();
}

void ImageJockeyGridPlot::setColorScaleMin(double value)
{
    m_colorScaleMin = value;
    setColorMap( ImageJockeyGridPlot::RGBMap );
	if( m_var )
		m_spectrumData->setInterval( Qt::ZAxis, QwtInterval( m_colorScaleMin, m_colorScaleMax ) );
	m_factorData->setInterval( Qt::ZAxis, QwtInterval( m_colorScaleMin, m_colorScaleMax ) );

    const QwtInterval zInterval = m_spectrogram->data()->interval( Qt::ZAxis );
    setAxisScale( QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue() );

    replot();
}

void ImageJockeyGridPlot::setDecibelRefValue(double value)
{
    m_spectrumData->setDecibelRefValue( value );
    setColorMap( ImageJockeyGridPlot::RGBMap );
    m_spectrumData->setInterval( Qt::ZAxis, QwtInterval( m_colorScaleMin, m_colorScaleMax ) );

    //update the z scale (color table ticks)
    const QwtInterval zInterval = m_spectrogram->data()->interval( Qt::ZAxis );
    setAxisScale( QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue() );

    replot();
}

void ImageJockeyGridPlot::draw1DSpectrogramBand()
{
    //get the object that triggered the call to this slot
    QObject* obj = sender();

    //check whether the event sender is a Spectrogram1DParameters
    Spectrogram1DParameters* spectr1DPar = qobject_cast<Spectrogram1DParameters*>( obj );
    if( ! spectr1DPar ){
		emit errorOccurred("ImageJockeyGridPlot::draw1DSpectrogramBand(): sender is not an Spectrogram1DParameters object. Nothing done.");
        return;
    }

    //set the curves geometry to reflect the geometry of the 1D Spectrogram calculation half-bands.
    m_curve1DSpectrogramHalfBand1->setSamples( spectr1DPar->get2DBand1Xs(),
                                               spectr1DPar->get2DBand1Ys(),
                                               spectr1DPar->getNPointsPerBandIn2DGeometry() );
    m_curve1DSpectrogramHalfBand2->setSamples( spectr1DPar->get2DBand2Xs(),
                                               spectr1DPar->get2DBand2Ys(),
                                               spectr1DPar->getNPointsPerBandIn2DGeometry() );
    replot();
}


