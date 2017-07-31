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

#include <qapplication.h>

#include "domain/attribute.h"
#include "domain/file.h"
#include "domain/application.h"
#include "domain/cartesiangrid.h"

class MyZoomer: public QwtPlotZoomer
{
public:
    MyZoomer( QWidget *canvas ):
        QwtPlotZoomer( canvas )
    {
        setTrackerMode( AlwaysOn );
    }

    virtual QwtText trackerTextF( const QPointF &pos ) const
    {
        QColor bg( Qt::white );
        bg.setAlpha( 200 );

        QwtText text = QwtPlotZoomer::trackerTextF( pos );
        text.setBackgroundBrush( QBrush( bg ) );
        return text;
    }
};

class SpectrogramData: public QwtRasterData{
public:
    SpectrogramData() : m_at( nullptr ), m_cg( nullptr ) {
        setInterval( Qt::XAxis, QwtInterval( -1.5, 1.5 ) );
        setInterval( Qt::YAxis, QwtInterval( -1.5, 1.5 ) );
        setInterval( Qt::ZAxis, QwtInterval( 0.0, 10.0 ) );
    }
    virtual double value( double x, double y ) const {
        if( ! m_at )
            return 0.0d;
        else{
            return 5.0d;
//            return m_cg->valueAt( m_at->getAttributeGEOEASgivenIndex()-1, x, y, 0);
        }
    }
    void setAttribute( Attribute* at ){
        m_at = at;
        if( at ){
            m_cg = (CartesianGrid*)at->getContainingFile(); //assumes Attribute's parent file is a Cartesian grid
//            setInterval( Qt::XAxis, QwtInterval( m_cg->getX0() - m_cg->getDX(),
//                                                 m_cg->getX0() + m_cg->getDX() * m_cg->getNX() ) );
//            setInterval( Qt::YAxis, QwtInterval( m_cg->getY0() - m_cg->getDY(),
//                                                 m_cg->getY0() + m_cg->getDY() * m_cg->getNY() ) );
//            setInterval( Qt::ZAxis, QwtInterval( m_cg->getZ0() - m_cg->getDZ(),
//                                                 m_cg->getZ0() + m_cg->getDZ() * m_cg->getNZ() ) );
        }
        else
            m_cg = nullptr;
    }
private:
    Attribute* m_at; //the attribute being displayed.
    CartesianGrid* m_cg; //the attribute's Cartesian grid.
};

class LinearColorMapRGB: public QwtLinearColorMap
{
public:
    LinearColorMapRGB():
        QwtLinearColorMap( Qt::darkCyan, Qt::red, QwtColorMap::RGB )
    {
        addColorStop( 0.1, Qt::cyan );
        addColorStop( 0.6, Qt::green );
        addColorStop( 0.95, Qt::yellow );
    }
};

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

class AlphaColorMap: public QwtAlphaColorMap
{
public:
    AlphaColorMap()
    {
        //setColor( QColor("DarkSalmon") );
        setColor( QColor("SteelBlue") );
    }
};

ImageJockeyGridPlot::ImageJockeyGridPlot( QWidget *parent ):
    QwtPlot( parent ),
    m_alpha(255),
    m_at( nullptr )
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
    rightAxis->setTitle( "Intensity" );
    rightAxis->setColorBarEnabled( true );

    setAxisScale( QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue() );
    enableAxis( QwtPlot::yRight );

    plotLayout()->setAlignCanvasToScales( true );

    setColorMap( ImageJockeyGridPlot::RGBMap );

    // LeftButton for the zooming
    // MidButton for the panning
    // RightButton: zoom out by 1
    // Ctrl+RighButton: zoom out to full size

    QwtPlotZoomer* zoomer = new MyZoomer( canvas() );
    zoomer->setMousePattern( QwtEventPattern::MouseSelect2,
        Qt::RightButton, Qt::ControlModifier );
    zoomer->setMousePattern( QwtEventPattern::MouseSelect3,
        Qt::RightButton );

    QwtPlotPanner *panner = new QwtPlotPanner( canvas() );
    panner->setAxisEnabled( QwtPlot::yRight, false );
    panner->setMouseButton( Qt::MidButton );

    // Avoid jumping when labels with more/less digits
    // appear/disappear when scrolling vertically

    const QFontMetrics fm( axisWidget( QwtPlot::yLeft )->font() );
    QwtScaleDraw *sd = axisScaleDraw( QwtPlot::yLeft );
    sd->setMinimumExtent( fm.width( "100.00" ) );

    const QColor c( Qt::darkBlue );
    zoomer->setRubberBandPen( c );
    zoomer->setTrackerPen( c );
}

void ImageJockeyGridPlot::setAttribute(Attribute *at)
{
    File *file = at->getContainingFile();
    if( file->getFileType() != "CARTESIANGRID" ){
        Application::instance()->logError("ImageJockeyGridPlot::setAttribute(): Attributes of " +
                                          file->getFileType() + " files not accepted.");
        m_spectrumData->setAttribute( nullptr );
    } else {
        m_spectrumData->setAttribute( at );
    }
    replot();
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
    const QwtInterval zInterval = m_spectrogram->data()->interval( Qt::ZAxis );

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


