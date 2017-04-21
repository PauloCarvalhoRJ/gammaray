#include "softindicatorcalibplot.h"
#include <QEvent>
#include <qwt_scale_widget.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_intervalcurve.h>
#include <qwt_legend.h>

#include "softindicatorcalibcanvaspicker.h"

SoftIndicatorCalibPlot::SoftIndicatorCalibPlot(QWidget *parent) :
    QwtPlot(parent),
    m_nCurves(2)
{
    setTitle( "Soft indicator calibration." );

    QwtPlotGrid *grid = new QwtPlotGrid;
    grid->setMajorPen( Qt::black, 0, Qt::DotLine );
    grid->attach( this );

    // axes
    setAxisScale( QwtPlot::xBottom, 0.0, 100.0 );
    setAxisScale( QwtPlot::yLeft, 0.0, 100.0 );

    setAxisTitle( QwtPlot::yLeft,
        QString( "Probability (%)" ) );

    // Avoid jumping when label with 3 digits
    // appear/disappear when scrolling vertically

    QwtScaleDraw *sd = axisScaleDraw( QwtPlot::yLeft );
    sd->setMinimumExtent( sd->extent( axisWidget( QwtPlot::yLeft )->font() ) );

    plotLayout()->setAlignCanvasToScales( true );

    insertCurve( Qt::Horizontal, Qt::yellow, 30.0, QString() );
    insertCurve( Qt::Horizontal, Qt::white, 70.0, QString() );

    replot();

    // we need the resize events, to lay out the wheel
    canvas()->installEventFilter( this );

    axisWidget( xBottom )->setWhatsThis(
        "Selecting a value at the scale will insert a new curve." );

    // The canvas picker handles all mouse and key
    // events on the plot canvas
    //( void ) new SoftIndicatorCalibCanvasPicker( this );
    SoftIndicatorCalibCanvasPicker* siccp = new SoftIndicatorCalibCanvasPicker( this ); //TODO: delete?
    //To be notified when a calibration curve is edited by the used.
    connect( siccp, SIGNAL(curveChanged(QwtPlotCurve*)), this, SLOT(onCurveChanged(QwtPlotCurve*)) );

    insertLegend( new QwtLegend() );
}

bool SoftIndicatorCalibPlot::eventFilter(QObject *object, QEvent *e)
{
    if ( e->type() == QEvent::Resize )
    {
        if ( object == axisWidget( yLeft ) )
        {
        }
        if ( object == canvas() )
        {
        }
    }

    return QwtPlot::eventFilter( object, e );
}

void SoftIndicatorCalibPlot::transferData(std::vector<double> &source)
{
    //remove current calibration curves
    clearCurves();

    //get the data
    m_data = std::move( source );

    //update horizontal axis scale
    setAxisScale( QwtPlot::xBottom, getDataMin(), getDataMax() );

    //set initial spacing between the curves
    double step = 100.0 / ( m_nCurves + 1 );

    //add the curves
    for( size_t i = 0; i < m_nCurves; ++i){
        insertCurve( Qt::Horizontal, step * (i+1));
    }
}

void SoftIndicatorCalibPlot::setNumberOfCurves(size_t number)
{
    m_nCurves = number;

    //remove current calibration curves
    clearCurves();

    //set the initial spacing between the curves
    double step = 100.0 / ( m_nCurves + 1 );

    //add the curves
    for( size_t i = 0; i < m_nCurves; ++i){
        insertCurve( Qt::Horizontal, step * (i+1));
    }
}

std::vector< std::vector<double> > SoftIndicatorCalibPlot::getSoftIndicators(SoftIndicatorCalculationMode mode)
{
    std::vector< std::vector<double> > m;

    if( mode == SoftIndicatorCalculationMode::CONTINUOUS ){
        //for each curve
        uint nCurves = m_curves.size();
        for( uint iCurve = 0; iCurve < nCurves; ++iCurve){
            QwtPlotCurve* curve = m_curves[iCurve];
            std::vector<double> softIndicators;
            softIndicators.reserve( m_data.size() );
            //for each datum
            uint nData = m_data.size();
            for( uint iDatum = 0; iDatum < nData; ++iDatum ){
                //find the index of the curve point whose X is less or equal than the data value, but the next
                // X is greater than.
                int iLeft = 0;
                for ( int i = 0; i < static_cast<int>( curve->dataSize() ); i++ )
                    if( m_data[iDatum] <= curve->sample(i).x() ){
                        iLeft = i-1;
                        //adjust for begining-of-scale case
                        if(iLeft < 0)
                            iLeft = 0;
                        break; //interrupt search
                    }
                //perform the linear interpolation
                double x = m_data[iDatum];
                double x0 = curve->sample(iLeft).x();
                double y0 = curve->sample(iLeft).y();
                double x1 = curve->sample(iLeft+1).x();
                double y1 = curve->sample(iLeft+1).y();
                double y = y0 + (x-x0) * (y1-y0) / (x1-x0);
                softIndicators.push_back( y );
            }
            m.push_back( softIndicators );
        }
    }

    if( mode == SoftIndicatorCalculationMode::CATEGORICAL ){
        //for each filled area
        uint nFills = m_fillAreas.size();
        for( uint iFill = 0; iFill < nFills; ++iFill){
            QwtPlotIntervalCurve* fill = m_fillAreas[iFill];
            std::vector<double> softIndicators;
            softIndicators.reserve( m_data.size() );
            //for each datum
            uint nData = m_data.size();
            for( uint iDatum = 0; iDatum < nData; ++iDatum ){
                //find the index of the fill area sample whose X is less or equal than the data value, but the next
                // X is greater than.
                int iLeft = 0;
                for ( int i = 0; i < static_cast<int>( fill->dataSize() ); i++ )
                    if( m_data[iDatum] <= fill->sample(i).value ){
                        iLeft = i-1;
                        //adjust for begining-of-scale case
                        if(iLeft < 0)
                            iLeft = 0;
                        break; //interrupt search
                    }
                //perform the linear interpolation of upper bound
                double x = m_data[iDatum];
                double x0 = fill->sample(iLeft).value;
                double y0 = fill->sample(iLeft).interval.maxValue();
                double x1 = fill->sample(iLeft+1).value;
                double y1 = fill->sample(iLeft+1).interval.maxValue();
                double yUpper = y0 + (x-x0) * (y1-y0) / (x1-x0);
                //perform the linear interpolation of lower bound
                y0 = fill->sample(iLeft).interval.minValue();
                y1 = fill->sample(iLeft+1).interval.minValue();
                double yLower = y0 + (x-x0) * (y1-y0) / (x1-x0);
                //stores the difference (probability of a category)
                softIndicators.push_back( yUpper - yLower );
            }
            m.push_back( softIndicators );
        }
    }

    //return by move semantics
    return std::move(m);
}

void SoftIndicatorCalibPlot::setXAxisLabel(QString text)
{
    setAxisTitle( QwtPlot::xBottom, text );
}

void SoftIndicatorCalibPlot::fillColor(const QColor &color, int base_curve, const QString label )
{
    //does nothing if there is no calibration curve.
    if( m_curves.size() == 0 )
        return;

    //get the number curve points (assumes all curves have the same number of points)
    int nData = m_curves[0]->data()->size();

    //correct for possibly out-of-range base curve index
    if( base_curve > (int)m_curves.size()-1 )
        base_curve = (int)m_curves.size()-1;
    if( base_curve < -1 )
        base_curve = -1;

    //get the index of the top curve
    int top_curve = base_curve + 1;

    //create a x, y_base, y_top sample collection object
    QVector<QwtIntervalSample> intervals( nData );

    //get the y intervals between the target curves
    for ( int i = 0; i < nData; ++i )
    {
        double low;
        double high;
        double x = 0.0;

        if( base_curve >= 0 ){
            low = m_curves[base_curve]->sample(i).y();
            x = m_curves[base_curve]->sample(i).x();
        }else
            low = 0.0;

        if( top_curve < (int)m_curves.size() ){
            high = m_curves[top_curve]->sample(i).y();
            x = m_curves[top_curve]->sample(i).x();
        }else
            high = 100.0;

        intervals[i] = QwtIntervalSample( x, QwtInterval( low, high ) );
    }

    //create and plot a interval curve object
    QwtPlotIntervalCurve *d_intervalCurve = new QwtPlotIntervalCurve( label );
    d_intervalCurve->setRenderHint( QwtPlotItem::RenderAntialiased );
    QColor bg( color );
    bg.setAlpha( 75 );
    d_intervalCurve->setBrush( QBrush( bg ) );
    d_intervalCurve->setStyle( QwtPlotIntervalCurve::Tube );
    d_intervalCurve->setSamples( intervals );
    d_intervalCurve->setLegendIconSize( QSize( 16, 16 ) );
    d_intervalCurve->attach( this );

    //store the pointer of the new interval curve
    m_fillAreas.push_back( d_intervalCurve );
}

void SoftIndicatorCalibPlot::setCurveLabel(int index, QString label)
{
    QwtPlotCurve* curve = m_curves[index];
    curve->setTitle( label );
    curve->setLegendIconSize( QSize( 16, 16 ) );
}

void SoftIndicatorCalibPlot::setCurveColor(int index, QColor color)
{
    QwtPlotCurve* curve = m_curves[index];
    curve->setPen( color, 3.0 );
}

void SoftIndicatorCalibPlot::setCurveBase(int index, double value)
{
    QwtPlotCurve* curve = m_curves[index];

    QVector<double> xData( curve->dataSize() );
    QVector<double> yData( curve->dataSize() );
    //for each curve point
    for ( int i = 0; i < static_cast<int>( curve->dataSize() ); i++ ) {
        const QPointF sample = curve->sample( i );
        xData[i] = sample.x();
        yData[i] = value;
    }
    //updates the curve points
    curve->setSamples( xData, yData );

    //prevents potential crossings
    pushCurves( curve );
 }

void SoftIndicatorCalibPlot::insertCurve(int axis, double base)
{
    Qt::Orientation o;
    if ( axis == yLeft || axis == yRight )
        o = Qt::Horizontal;
    else
        o = Qt::Vertical;

    //QRgb rgb = static_cast<QRgb>( (uint)( (double) std::rand() / (double)RAND_MAX * (1U<<31)) );
    //insertCurve( o, QColor( rgb ), base, QString() );
    insertCurve( o, Qt::black, base, QString() );
    replot();
}

void SoftIndicatorCalibPlot::insertCurve(Qt::Orientation o, const QColor &c, double base, QString label )
{
    QwtPlotCurve *curve = new QwtPlotCurve( label );

    curve->setPen( c );
    curve->setSymbol( new QwtSymbol( QwtSymbol::Ellipse,
        Qt::gray, c, QSize( 8, 8 ) ) );

    size_t nPoints = 11;
    double x[nPoints];
    double y[nPoints];

    double step = ( getDataMax() - getDataMin() ) / (nPoints - 1);

    double dataMin = getDataMin();

    for ( uint i = 0; i < nPoints; i++ )
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

    curve->setSamples( x, y, nPoints );
    curve->attach( this );

    //put legend icon if label text is set
    if( ! label.isEmpty() )
        curve->setLegendIconSize( QSize( 16, 16 ) );
    else
        curve->setLegendIconSize( QSize( 0, 0 ) );

    m_curves.push_back( curve );
}

void SoftIndicatorCalibPlot::clearCurves()
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

void SoftIndicatorCalibPlot::clearFillAreas()
{
    std::vector<QwtPlotIntervalCurve*>::iterator it = m_fillAreas.begin();
    for(; it != m_fillAreas.end(); ++it){
        (*it)->detach( );
        //delete *it; //assuming autoDelete is on for this QwtPlot
    }
    m_fillAreas.clear();
}

void SoftIndicatorCalibPlot::updateFillAreas()
{
    //does nothing if there is no calibration curve.
    if( m_curves.size() == 0 )
        return;

    //get the number curve points (assumes all curves have the same number of points)
    int nPoints = m_curves[0]->data()->size();

    //get number of fill areas between
    int nFills = m_fillAreas.size();

    //does nothing if there are no fill areas (continuous variable case)
    if( nFills == 0 )
        return;

    for( int iFill = 0; iFill < nFills; ++iFill){

        //get the index of the base curve (can be -1 to indicate the base is constant at 0.0%)
        int base_curve = iFill - 1;

        //get the index of the top curve
        int top_curve = base_curve + 1;

        //create a x, y_base, y_top sample collection object
        QVector<QwtIntervalSample> intervals( nPoints );

        //get the y intervals between the target curves
        for ( int i = 0; i < nPoints; ++i )
        {
            double low;
            double high;
            double x = 0.0;

            if( base_curve >= 0 ){
                low = m_curves[base_curve]->sample(i).y();
                x = m_curves[base_curve]->sample(i).x();
            }else
                low = 0.0;

            if( top_curve < (int)m_curves.size() ){
                high = m_curves[top_curve]->sample(i).y();
                x = m_curves[top_curve]->sample(i).x();
            }else
                high = 100.0;

            intervals[i] = QwtIntervalSample( x, QwtInterval( low, high ) );
        }

        //update the fill area geometry
        m_fillAreas[iFill]->setSamples( intervals );
    }

}

void SoftIndicatorCalibPlot::pushCurves(QwtPlotCurve *curve)
{
    std::vector<QwtPlotCurve*> curvesAbove;
    std::vector<QwtPlotCurve*> curvesBelow;

    //get the index of the changed curve;
    std::vector<QwtPlotCurve*>::iterator it = m_curves.begin();
    int indexOfCurve = 0;
    for(; it != m_curves.end(); ++it, ++indexOfCurve)
        if( *it == curve ){
            //++indexOfCurve;
            break;
        }

    //get the curves above the changed curve
    for( size_t i = indexOfCurve+1; i < m_curves.size(); ++i)
        curvesAbove.push_back( m_curves[i] );

    //get the curves below the changed curve
    for( int i = indexOfCurve-1; i >= 0; --i)
        curvesBelow.push_back( m_curves[i] );

    //push upwards all curves above the changed curve
    it = curvesAbove.begin();
    for(; it != curvesAbove.end(); ++it){
        QVector<double> xData( (*it)->dataSize() );
        QVector<double> yData( (*it)->dataSize() );
        //for each curve point
        for ( int i = 0; i < static_cast<int>( (*it)->dataSize() ); i++ ) {
            const QPointF sample = (*it)->sample( i );
            xData[i] = sample.x();
            yData[i] = sample.y();
            //if the Y of the curve above is lower, forces it to equal the changed curve
            if( yData[i] < curve->sample(i).y() )
                yData[i] = curve->sample(i).y();
        }
        //updates the curve points
        (*it)->setSamples( xData, yData );
    }

    //push downwards all curves below the changed curve
    it = curvesBelow.begin();
    for(; it != curvesBelow.end(); ++it){
        QVector<double> xData( (*it)->dataSize() );
        QVector<double> yData( (*it)->dataSize() );
        //for each curve point
        for ( int i = 0; i < static_cast<int>( (*it)->dataSize() ); i++ ) {
            const QPointF sample = (*it)->sample( i );
            xData[i] = sample.x();
            yData[i] = sample.y();
            //if the Y of the curve above is higher, forces it to equal the changed curve
            if( yData[i] > curve->sample(i).y() )
                yData[i] = curve->sample(i).y();
        }
        //updates the curve points
        (*it)->setSamples( xData, yData );
    }
}

double SoftIndicatorCalibPlot::getDataMax()
{
    if( m_data.size() > 0 ){
        std::vector<double>::iterator it = std::max_element( m_data.begin(), m_data.end() );
        return *it;
    } else {
        return 100.0;
    }
}

double SoftIndicatorCalibPlot::getDataMin()
{
    if( m_data.size() > 0 ){
        std::vector<double>::iterator it = std::min_element( m_data.begin(), m_data.end() );
        return *it;
    } else {
        return 0.0;
    }
}

void SoftIndicatorCalibPlot::onCurveChanged( QwtPlotCurve *changed_curve )
{
    pushCurves( changed_curve );
    updateFillAreas();
}
