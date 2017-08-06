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

#include "domain/attribute.h"
#include "domain/file.h"
#include "domain/application.h"
#include "domain/cartesiangrid.h"
#include "util.h"
#include "spectrogram1dparameters.h"
#include "geostats/matrix3x3.h"
#include "geostats/geostatsutils.h"

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
    SpectrogramData() : m_at( nullptr ), m_cg( nullptr ), m_decibelRefValue(100.0) {
        //set some default values before the user chooses a grid
        setInterval( Qt::XAxis, QwtInterval( -1.5, 1.5 ) );
        setInterval( Qt::YAxis, QwtInterval( -1.5, 1.5 ) );
        setInterval( Qt::ZAxis, QwtInterval( 0.0, 10.0 ) );
    }
    /** Returns a grid value as a function of spatial location.
     *  Returns NaN if the location is outside the grid or corresponds to an unvalued cell.
     */
    virtual double value( double x, double y ) const {
        if( ! m_at ) //no grid selected
            //returning a NaN means a blank plot
            return std::numeric_limits<double>::quiet_NaN();
        else{
            // get the grid value as is
            double value = m_cg->valueAt( m_at->getAttributeGEOEASgivenIndex()-1, x, y, 0);
            if( m_cg->isNDV(value) ) //if there is no value there
                value = std::numeric_limits<double>::quiet_NaN(); //returns NaN (blank plot)
            else
                //for Fourier images, get the absolute values in decibel for ease of interpretation
                value = Util::dB( std::abs<double>(value), m_decibelRefValue, 0.0000001 );
            return value;
        }
    }
    /** Define the Attribute of a Cartesian grid.  Resets the plot. */
    void setAttribute( Attribute* at ){
        m_at = at;
        if( at ){
            //assumes Attribute's parent file is a Cartesian grid
            m_cg = (CartesianGrid*)at->getContainingFile();
            //get the column number in the GEO-EAS file corresponding to the Attribute
            int columnIndex = at->getAttributeGEOEASgivenIndex() - 1;
            //resets the map display coverage to show the entire map
            setInterval( Qt::XAxis, QwtInterval( m_cg->getX0() - m_cg->getDX(),
                                                 m_cg->getX0() + m_cg->getDX() * m_cg->getNX() ) );
            setInterval( Qt::YAxis, QwtInterval( m_cg->getY0() - m_cg->getDY(),
                                                 m_cg->getY0() + m_cg->getDY() * m_cg->getNY() ) );
            //load data from file
            m_cg->loadData();
            //for Fourier images, get the absolute values in decibel for ease of interpretation
            double min = Util::dB( m_cg->minAbs( columnIndex ), m_decibelRefValue, 0.0000001 );
            double max = Util::dB( m_cg->maxAbs( columnIndex ), m_decibelRefValue, 0.0000001 );
            //Z in a 2D raster plot is the attribute value, not the Z coordinate.
            setInterval( Qt::ZAxis, QwtInterval( min, max ));
        }
        else
            m_cg = nullptr; //resets Cartesian grid pointer if Attribute is null
    }
    /** Defines the reference value for decibel scaling. */
    void setDecibelRefValue( double value ){
        m_decibelRefValue = value;
    }
private:
    /** The attribute being displayed. */
    Attribute* m_at;
    /** The attribute's Cartesian grid. */
    CartesianGrid* m_cg;
    /** The reference value for decibel scaling. */
    double m_decibelRefValue;
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
    m_at( nullptr ),
    m_zoomer( nullptr ),
    m_colorScaleMax( 10.0 ),
    m_colorScaleMin( 0.0 ),
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

void ImageJockeyGridPlot::setAttribute(Attribute *at)
{
    //get the data
    File *file = at->getContainingFile();
    if( file->getFileType() != "CARTESIANGRID" ){
        Application::instance()->logError("ImageJockeyGridPlot::setAttribute(): Attributes of " +
                                          file->getFileType() + " files not accepted.");
        m_spectrumData->setAttribute( nullptr );
    } else {
        m_spectrumData->setAttribute( at );
    }

    //redefine color scale/legend
    const QwtInterval zInterval = m_spectrogram->data()->interval( Qt::ZAxis );
    // A color bar on the right axis
    QwtScaleWidget *rightAxis = axisWidget( QwtPlot::yRight );
    rightAxis->setTitle( at->getName() + " (dB)" );
    setAxisScale( QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue() );
    setColorMap( ImageJockeyGridPlot::RGBMap );

    //reset the zoom stack to the possibly new geographic region.
    m_zoomer->setZoomBase();

    replot();
}

double ImageJockeyGridPlot::getScaleMaxValue()
{
    return m_spectrogram->data()->interval( Qt::ZAxis ).maxValue();
}

double ImageJockeyGridPlot::getScaleMinValue()
{
    return m_spectrogram->data()->interval( Qt::ZAxis ).minValue();
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
    m_spectrumData->setInterval( Qt::ZAxis, QwtInterval( m_colorScaleMin, m_colorScaleMax ) );

    const QwtInterval zInterval = m_spectrogram->data()->interval( Qt::ZAxis );
    setAxisScale( QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue() );

    replot();
}

void ImageJockeyGridPlot::setColorScaleMin(double value)
{
    m_colorScaleMin = value;
    setColorMap( ImageJockeyGridPlot::RGBMap );
    m_spectrumData->setInterval( Qt::ZAxis, QwtInterval( m_colorScaleMin, m_colorScaleMax ) );

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

    //check whether it is a Spectrogram1DParameters
    Spectrogram1DParameters* spectr1DPar = qobject_cast<Spectrogram1DParameters*>( obj );
    if( ! spectr1DPar ){
        Application::instance()->logError("ImageJockeyGridPlot::draw1DSpectrogramBand(): sender is not an Spectrogram1DParameters object. Nothing done.");
        return;
    }

    m_curve1DSpectrogramHalfBand1->setSamples( spectr1DPar->get2DBand1Xs(),
                                               spectr1DPar->get2DBand1Ys(),
                                               spectr1DPar->getNPointsPerBandIn2DGeometry() );

    m_curve1DSpectrogramHalfBand2->setSamples( spectr1DPar->get2DBand2Xs(),
                                               spectr1DPar->get2DBand2Ys(),
                                               spectr1DPar->getNPointsPerBandIn2DGeometry() );

    replot();
}


