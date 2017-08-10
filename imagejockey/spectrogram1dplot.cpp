#include "spectrogram1dplot.h"

#include <qwt_plot_canvas.h>
#include <qwt_scale_widget.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/scoped_array.hpp>

#include "domain/attribute.h"
#include "domain/application.h"
#include "domain/cartesiangrid.h"
#include "spectrogram1dparameters.h"
#include "util.h"


///================================THE VISUAL GRID PATTERN FOR THE 1D SPECTROGRAM===============
class Spectrogram1DGrid: public QwtPlotGrid
{
public:
    Spectrogram1DGrid()
    {
        enableXMin( true );
        enableYMin( true );
        setMajorPen( Qt::green, 0, Qt::DotLine );
        setMinorPen( Qt::darkGreen, 0, Qt::DotLine );
    }

    virtual void updateScaleDiv(
            const QwtScaleDiv &xScaleDiv,
            const QwtScaleDiv &yScaleDiv )
    {
        QwtScaleDiv scaleDiv( xScaleDiv.lowerBound(),
                              xScaleDiv.upperBound() );
        scaleDiv.setTicks( QwtScaleDiv::MinorTick,
            xScaleDiv.ticks( QwtScaleDiv::MinorTick ) );
        scaleDiv.setTicks( QwtScaleDiv::MajorTick,
            xScaleDiv.ticks( QwtScaleDiv::MediumTick ) );

        QwtScaleDiv scaleDivY( yScaleDiv.lowerBound(),
                              yScaleDiv.upperBound() );
        scaleDivY.setTicks( QwtScaleDiv::MinorTick,
            yScaleDiv.ticks( QwtScaleDiv::MinorTick ) );
        scaleDivY.setTicks( QwtScaleDiv::MajorTick,
            yScaleDiv.ticks( QwtScaleDiv::MediumTick ) );

        QwtPlotGrid::updateScaleDiv( xScaleDiv, scaleDivY );
    }
};

///=============================THE SPECTROGRAM1DPLOT ITSELF===============================

Spectrogram1DPlot::Spectrogram1DPlot(QWidget *parent) :
    QwtPlot( parent ),
    m_at( nullptr ),
    m_curve( new QwtPlotCurve() ), //TODO: this should be deleted in the destructor
    m_decibelRefValue(1000.0), //1000.0 intial reference for dB has no special meaning
    m_yScaleMax(20), //init dB scale max of 20 fits most cases
    m_yScaleMin(-150), //init dB scale min of -150 fits most cases
    m_xScaleMax(1000.0), //init frquency scale max with a reasonable value
    m_frequencyWindowBeginCurve( new QwtPlotCurve() ), //TODO: this should be deleted in the destructor
    m_frequencyWindowEndCurve( new QwtPlotCurve() ), //TODO: this should be deleted in the destructor
    m_freqWindowBegin(0.0),
    m_freqWindowEnd(1000.0)
{
    QwtPlotCanvas *canvas = new QwtPlotCanvas();
    canvas->setPalette( Qt::black );
    setCanvas( canvas );

    // plot grid
    QwtPlotGrid *grid = new Spectrogram1DGrid();
    grid->attach( this );

    // visual object containing the sample points
    m_curve->setRenderHint( QwtPlotItem::RenderAntialiased );
    m_curve->setStyle( QwtPlotCurve::Dots );
    m_curve->setPen( Qt::green, 0 );
    m_curve->attach( this );

    //the two vertical lines represting the frequency window for the equalizer
    m_frequencyWindowBeginCurve->setStyle( QwtPlotCurve::Lines );
    m_frequencyWindowBeginCurve->setPen( Qt::green, 0 );
    m_frequencyWindowBeginCurve->attach( this );
    m_frequencyWindowEndCurve->setStyle( QwtPlotCurve::Lines );
    m_frequencyWindowEndCurve->setPen( Qt::green, 0 );
    m_frequencyWindowEndCurve->attach( this );

    //axes text styles
    setAxisTitle( QwtPlot::yLeft, "<span style=\" font-size:7pt;\">info. contr. (dB)</span>");
    setAxisTitle( QwtPlot::xBottom, "<span style=\" font-size:7pt;\">spatial frequency (feature size <sup>-1</sup>)</span>");
    QwtScaleWidget* xaxisw = axisWidget( Axis::yLeft );
    xaxisw->setStyleSheet("font: 7pt;");
    QwtScaleWidget* yaxisw = axisWidget( Axis::xBottom );
    yaxisw->setStyleSheet("font: 7pt;");
}

void Spectrogram1DPlot::setAttribute(Attribute *at)
{
    m_at = at;
    if( ! m_at ){
        return;
    }
    //assumes the Attribute's parent file is a Cartesian grid
    CartesianGrid* cg = (CartesianGrid*)m_at->getContainingFile();
    setHorizontalScaleMax( cg->getDiagonalLength() / 2.0 );
}

void Spectrogram1DPlot::rereadSpectrogramData()
{
    //some typedefs to shorten code
    typedef boost::geometry::model::d2::point_xy<double> boostPoint2D;
    typedef boost::geometry::model::polygon<boostPoint2D> boostPolygon;

    //check whether we the necessary data
    if( ! m_at ){
        Application::instance()->logError("Spectrogram1DPlot::rereadSpectrogramData(): Attribute is null.  Nothing done.");
        return;
    }

    //get the attribute's GEO-EAS column index
    uint columnIndex = m_at->getAttributeGEOEASgivenIndex() - 1;

    //get the object that triggered the call to this slot
    QObject* obj = sender();

    //check whether the event sender is a Spectrogram1DParameters
    Spectrogram1DParameters* spectr1DPar = qobject_cast<Spectrogram1DParameters*>( obj );
    if( ! spectr1DPar ){
        Application::instance()->logError("Spectrogram1DPlot::rereadSpectrogramData(): sender is not an Spectrogram1DParameters object. Nothing done.");
        return;
    }

    //assumes the Attribute's parent file is a Cartesian grid
    CartesianGrid* cg = (CartesianGrid*)m_at->getContainingFile();

    //TODO: add support for rotations
    if( ! Util::almostEqual2sComplement( cg->getRot(), 0.0, 1) ){
        Application::instance()->logError("Spectrogram1DPlot::rereadSpectrogramData(): rotation not supported yet.  Nothing done.");
        return;
    }

    //define a Boost polygon from the 1D spectrogram calculation half-band geometry (assumes the 2D spectrogram is symmetrical)
    const std::size_t n = spectr1DPar->getNPointsPerBandIn2DGeometry();
    boost::scoped_array<boostPoint2D> points(new boostPoint2D[n]); //scoped_array frees memory when its scope ends.
    for(std::size_t i = 0; i < n; ++i)
        points[i] = boostPoint2D(spectr1DPar->get2DBand1Xs()[i], spectr1DPar->get2DBand1Ys()[i]);
    boostPolygon poly;
    boost::geometry::assign_points( poly, std::make_pair(&points[0], &points[0] + n));

    //this list contains pairs of values ready for 1D spectrogram display
    //the X value is the spatial frequency (distance from the center of the 2D spectrogram grid)
    //the Y value is the intensity (variable value in the grid, normaly in decibel scale)
    QVector<QPointF> spectrogram1Dsamples;

    //scan the grid, testing each cell whether it lies in the 1D spectrogram calculation band.
    //TODO: this code assumes no grid rotation and that the grid is 2D.
    SpatialLocation gridCenter = cg->getCenter();
    for( uint k = 0; k < cg->getNZ(); ++k ){
        // z coordinate is ignored in 2D spectrograms
        for( uint j = 0; j < cg->getNY(); ++j ){
            double cellCenterY = cg->getY0() + j * cg->getDY();
            for( uint i = 0; i < cg->getNX(); ++i ){
                double cellCenterX = cg->getX0() + i * cg->getDX();
                boostPoint2D p(cellCenterX, cellCenterY);
                // if the cell center lies within the 1D spectrogram calculation band
                // the distance-to-axis test runs faster than the point-in-poly test, so it allows a faster
                // discard of grid cells obviously outside the band.  The 1D spectrogram then refreshes faster
                // with narrower bands.
                if(     spectr1DPar->distanceToAxis(cellCenterX, cellCenterY) <= spectr1DPar->bandWidth()
                        &&
                        boost::geometry::within(p, poly)
                        ){
                    double intensity;
                    // get the grid value as is
                    double value = cg->dataIJK( columnIndex, i, j, k );
                    // calculate the intensity value from the raw spectrogram value
                    if( cg->isNDV(value) ) //if there is no value there
                        intensity = std::numeric_limits<double>::quiet_NaN(); //intensity is NaN (blank plot)
                    else
                        //for Fourier images, get the absolute values in decibel for ease of interpretation
                        intensity = Util::dB( std::abs<double>(value), m_decibelRefValue, 0.0000001 );
                    // get the distance orthogonal distance components
                    double dX = cellCenterX - gridCenter._x;
                    double dY = cellCenterY - gridCenter._y;
                    // the spatial frequency in a spectrogram is proportional to the distance from its center
                    // the spatial frequency is the inverse of feature size
                    double spatialFrequency = std::sqrt<double>( dX*dX + dY*dY ).real();
                    // store the pair for plot
                    spectrogram1Dsamples.push_back( QPointF(spatialFrequency, intensity) );
                }
            }
        }
    }

    //plot the 1D spectrogram corresponding to a band over the 2D spectrogram
    m_curve->setSamples( spectrogram1Dsamples );
    replot();
}

void Spectrogram1DPlot::setDecibelRefValue(double value)
{
    m_decibelRefValue = value;
    replot();
}

void Spectrogram1DPlot::setVerticalScaleMax(double value)
{
    m_yScaleMax = value;
    setAxisScale( QwtPlot::yLeft, m_yScaleMin, m_yScaleMax );
    updateFrequencyWindowLines();
    replot();
}

void Spectrogram1DPlot::setVerticalScaleMin(double value)
{
    m_yScaleMin = value;
    setAxisScale( QwtPlot::yLeft, m_yScaleMin, m_yScaleMax );
    updateFrequencyWindowLines();
    replot();
}

void Spectrogram1DPlot::setHorizontalScaleMax(double value)
{
    m_xScaleMax = value;
    setAxisScale( QwtPlot::xBottom, 0, m_xScaleMax );
    updateFrequencyWindowLines();
    replot();
}

void Spectrogram1DPlot::updateFrequencyWindow(double begin, double end)
{
    m_freqWindowBegin = begin;
    m_freqWindowEnd = end;
    updateFrequencyWindowLines();
}

void Spectrogram1DPlot::updateFrequencyWindowLines( )
{
    QVector<QPointF> points;
    points.push_back( QPointF( m_freqWindowBegin, m_yScaleMin ) );
    points.push_back( QPointF( m_freqWindowBegin, m_yScaleMax ) );
    m_frequencyWindowBeginCurve->setSamples( points );
    points.clear();
    points.push_back( QPointF( m_freqWindowEnd, m_yScaleMin ) );
    points.push_back( QPointF( m_freqWindowEnd, m_yScaleMax ) );
    m_frequencyWindowEndCurve->setSamples( points );
    replot();
}
