#include "verticalproportioncurvesplot.h"
#include "vertpropcurves/verticalproportioncurvescanvaspicker.h"

#include <qwt_plot_grid.h>
#include <qwt_scale_widget.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>
#include <qwt_plot_intervalcurve.h>
#include <qwt_legend.h>

#include "util.h"

VerticalProportionCurvesPlot::VerticalProportionCurvesPlot( QWidget *parent ) :
    QwtPlot( parent ),
    m_nCurves(2),
    m_nPoints(11),
    m_handleSize(8),
    m_legendIconSize(16),
    m_isEditable( false ),
    m_VCPPicker( nullptr )
{
    setTitle( "Vertical proportion curves" );

    //set icon and curves handles sizes depending on display resolution
    if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI ) {
        m_handleSize = 16;
        m_legendIconSize = 32;
    }

    QwtPlotGrid *grid = new QwtPlotGrid;
    grid->setMajorPen( Qt::black, 0, Qt::DotLine );
    grid->attach( this );

    // axes
    setAxisScale( QwtPlot::xBottom, 0.0, 100.0 );
    setAxisTitle( QwtPlot::xBottom, QString( "proportion (%)" ) );
    setAxisScale( QwtPlot::yLeft, 0.0, 100.0 );
    setAxisTitle( QwtPlot::yLeft, QString( "base-top depth fraction (%)" ) );

    // Avoid jumping when label with 3 digits
    // appear/disappear when scrolling vertically
    QwtScaleDraw *sd = axisScaleDraw( QwtPlot::yLeft );
    sd->setMinimumExtent( sd->extent( axisWidget( QwtPlot::yLeft )->font() ) );

    plotLayout()->setAlignCanvasToScales( true );

    insertCurve( Qt::Vertical, Qt::yellow, 30.0, QString() );
    insertCurve( Qt::Vertical, Qt::white, 70.0, QString() );

    replot();

    insertLegend( new QwtLegend() );
}

void VerticalProportionCurvesPlot::setNumberOfCurves(size_t number)
{
    m_nCurves = number;

    //remove current calibration curves
    clearCurves();

    //set the initial spacing between the curves
    double step = 100.0 / ( m_nCurves + 1 );

    //add the curves
    for( size_t i = 0; i < m_nCurves; ++i){
        insertCurve( Qt::Vertical, step * (i+1) );
    }
}

void VerticalProportionCurvesPlot::fillColor(const QColor &color, int base_curve, const QString label)
{
    //does nothing if there is no proportion curve.
    if( m_curves.size() == 0 )
        return;

    //get the number curve points (assumes all curves have the same number of points)
    int nData = m_curves[0]->data()->size();

    //correct for possibly out-of-range base curve index
    if( base_curve > (int)m_curves.size()-1 )
        base_curve = (int)m_curves.size()-1;
    if( base_curve < -1 )
        base_curve = -1;

    //get the index of the curve to the right
    int right_curve = base_curve + 1;

    //create a y, x_base, x_right sample collection object
    QVector<QwtIntervalSample> intervals( nData );

    //get the x intervals between the target curves
    for ( int i = 0; i < nData; ++i )
    {
        double left;
        double right;
        double y = 0.0;

        if( base_curve >= 0 ){
            left = m_curves[base_curve]->sample(i).x();
            y = m_curves[base_curve]->sample(i).y();
        }else
            left = 0.0;

        if( right_curve < (int)m_curves.size() ){
            right = m_curves[right_curve]->sample(i).x();
            y = m_curves[right_curve]->sample(i).y();
        }else
            right = 100.0;

        intervals[i] = QwtIntervalSample( y, QwtInterval( left, right ) );
    }

    //create and plot a interval curve object
    QwtPlotIntervalCurve *d_intervalCurve = new QwtPlotIntervalCurve( label );
    d_intervalCurve->setRenderHint( QwtPlotItem::RenderAntialiased );
    QColor bg( color );
    bg.setAlpha( 90 );
    d_intervalCurve->setBrush( QBrush( bg ) );
    d_intervalCurve->setStyle( QwtPlotIntervalCurve::Tube );
    d_intervalCurve->setOrientation( Qt::Horizontal );
    d_intervalCurve->setSamples( intervals );
    d_intervalCurve->setLegendIconSize( QSize( m_legendIconSize, m_legendIconSize ) );
    d_intervalCurve->setPen( QColor(), 0.0, Qt::NoPen );
    d_intervalCurve->attach( this );

    //store the pointer of the new interval curve
    m_fillAreas.push_back( d_intervalCurve );
}

void VerticalProportionCurvesPlot::setCurveBase(int index, double value)
{
    QwtPlotCurve* curve = m_curves[index];

    QVector<double> xData( curve->dataSize() );
    QVector<double> yData( curve->dataSize() );
    //for each curve point
    for ( int i = 0; i < static_cast<int>( curve->dataSize() ); i++ ) {
        const QPointF sample = curve->sample( i );
        xData[i] = value;
        yData[i] = sample.y();
    }

    //updates the curve points
    curve->setSamples( xData, yData );

    //prevents potential crossings
    pushCurves( curve );
}

void VerticalProportionCurvesPlot::updateFillAreas()
{
    //does nothing if there is no proportion curve.
    if( m_curves.size() == 0 )
        return;

    //get the number curve points (assumes all curves have the same number of points)
    int nPoints = m_curves[0]->data()->size();

    //get number of fill areas between
    int nFills = m_fillAreas.size();

    //does nothing if there are no fill areas
    if( nFills == 0 )
        return;

    for( int iFill = 0; iFill < nFills; ++iFill){

        //get the index of the base curve (can be -1 to indicate the base is constant at 0.0%)
        int base_curve = iFill - 1;

        //get the index of the top curve
        int right_curve = base_curve + 1;

        //create a y, x_base, x_right sample collection object
        QVector<QwtIntervalSample> intervals( nPoints );

        //get the x intervals between the target curves
        for ( int i = 0; i < nPoints; ++i )
        {
            double left;
            double right;
            double y = 0.0;

            if( base_curve >= 0 ){
                left = m_curves[base_curve]->sample(i).x();
                y = m_curves[base_curve]->sample(i).y();
            }else
                left = 0.0;

            if( right_curve < (int)m_curves.size() ){
                right = m_curves[right_curve]->sample(i).x();
                y = m_curves[right_curve]->sample(i).y();
            }else
                right = 100.0;

            intervals[i] = QwtIntervalSample( y, QwtInterval( left, right ) );
        }

        //update the fill area geometry
        m_fillAreas[iFill]->setSamples( intervals );
    }
}

void VerticalProportionCurvesPlot::setEditable(bool value)
{
    m_isEditable = value;

    showHideCurveHandles();

    if( m_isEditable ){
        // The canvas picker handles all mouse and key
        // events on the plot canvas
        m_VCPPicker = new VerticalProportionCurvesCanvasPicker( this );
        //To be notified when a calibration curve is edited by the user.
        connect( m_VCPPicker, SIGNAL(curveChanged(QwtPlotCurve*)), this, SLOT(onCurveChanged(QwtPlotCurve*)) );
    } else
        delete m_VCPPicker;

    replot();
}

void VerticalProportionCurvesPlot::setNumberOfPoints(int number)
{
    m_nPoints = number;
}

void VerticalProportionCurvesPlot::insertCurve(int axis, double base)
{
    Qt::Orientation o;
    if ( axis == yLeft || axis == yRight )
        o = Qt::Horizontal;
    else
        o = Qt::Vertical;

    insertCurve( o, Qt::black, base, QString() );
    replot();
}

void VerticalProportionCurvesPlot::insertCurve(Qt::Orientation o, const QColor &c, double base, QString label)
{
    QwtPlotCurve *curve = new QwtPlotCurve( label );

    curve->setPen( c, 0.0, Qt::NoPen );

    double x[m_nPoints];
    double y[m_nPoints];

    //min and max in both axes are always 0.0 and 100.0 (%)
    double step = 100.0 / (m_nPoints - 1);

    //min in both axes is always 0.0 (%)
    double dataMin = 0.0;

    for ( uint i = 0; i < m_nPoints; i++ )
    {
        double v = dataMin + i * step;
        if ( o == Qt::Horizontal )
        {
            x[i] = v;
            y[i] = base;
        }
        else
        {
            x[i] = base;
            y[i] = v;
        }
    }

    curve->setSamples( x, y, m_nPoints );
    curve->attach( this );

    //put legend icon if label text is set
    if( ! label.isEmpty() )
        curve->setLegendIconSize( QSize( m_legendIconSize, m_legendIconSize ) );
    else
        curve->setLegendIconSize( QSize( 0, 0 ) );

    m_curves.push_back( curve );

    showHideCurveHandles();
}

void VerticalProportionCurvesPlot::clearCurves()
{
    std::vector<QwtPlotCurve*>::iterator it = m_curves.begin();
    for(; it != m_curves.end(); ++it){
        (*it)->detach( );
        //delete *it;  //assuming autoDelete is on for this QwtPlot
    }
    m_curves.clear();

    //also clears the filled areas between the curves, if any.
    clearFillAreas();
}

void VerticalProportionCurvesPlot::clearFillAreas()
{
    std::vector<QwtPlotIntervalCurve*>::iterator it = m_fillAreas.begin();
    for(; it != m_fillAreas.end(); ++it){
        (*it)->detach( );
        //delete *it; //assuming autoDelete is on for this QwtPlot
    }
    m_fillAreas.clear();
}

void VerticalProportionCurvesPlot::pushCurves(QwtPlotCurve *curve)
{
    std::vector<QwtPlotCurve*> curvesRight;
    std::vector<QwtPlotCurve*> curvesLeft;

    //get the index of the changed curve;
    std::vector<QwtPlotCurve*>::iterator it = m_curves.begin();
    int indexOfCurve = 0;
    for(; it != m_curves.end(); ++it, ++indexOfCurve)
        if( *it == curve ){
            //++indexOfCurve;
            break;
        }

    //get the curves to the right of the changed curve
    for( size_t i = indexOfCurve+1; i < m_curves.size(); ++i)
        curvesRight.push_back( m_curves[i] );

    //get the curves to the left of the changed curve
    for( int i = indexOfCurve-1; i >= 0; --i)
        curvesLeft.push_back( m_curves[i] );

    //push to the right all curves to the right of the changed curve
    it = curvesRight.begin();
    for(; it != curvesRight.end(); ++it){
        QVector<double> xData( (*it)->dataSize() );
        QVector<double> yData( (*it)->dataSize() );
        //for each curve point
        for ( int i = 0; i < static_cast<int>( (*it)->dataSize() ); i++ ) {
            const QPointF sample = (*it)->sample( i );
            xData[i] = sample.x();
            yData[i] = sample.y();
            //if the X of the curve to the right is lower, forces it to equal the X of the changed curve
            if( xData[i] < curve->sample(i).x() )
                xData[i] = curve->sample(i).x();
        }
        //updates the curve points
        (*it)->setSamples( xData, yData );
    }

    //push to the left all curves to the left of the changed curve
    it = curvesLeft.begin();
    for(; it != curvesLeft.end(); ++it){
        QVector<double> xData( (*it)->dataSize() );
        QVector<double> yData( (*it)->dataSize() );
        //for each curve point
        for ( int i = 0; i < static_cast<int>( (*it)->dataSize() ); i++ ) {
            const QPointF sample = (*it)->sample( i );
            xData[i] = sample.x();
            yData[i] = sample.y();
            //if the X of the curve to the left is higher, forces it to equal the X of the changed curve
            if( xData[i] > curve->sample(i).x() )
                xData[i] = curve->sample(i).x();
        }
        //updates the curve points
        (*it)->setSamples( xData, yData );
    }
}

void VerticalProportionCurvesPlot::showHideCurveHandles()
{
    std::vector<QwtPlotCurve*>::iterator it = m_curves.begin();
    for(; it != m_curves.end(); ++it){
        if( m_isEditable )
            (*it)->setSymbol( new QwtSymbol( QwtSymbol::Ellipse,
                                             Qt::gray,
                                             (*it)->pen().color(),
                                             QSize( m_handleSize, m_handleSize ) ) );
        else
            (*it)->setSymbol( new QwtSymbol( QwtSymbol::NoSymbol ) ); //the curve deletes the previous QwtSymbol object as it takes ownsership of the pointer.
    }
}

void VerticalProportionCurvesPlot::onCurveChanged(QwtPlotCurve *changed_curve)
{
    pushCurves( changed_curve );
    updateFillAreas();
}
