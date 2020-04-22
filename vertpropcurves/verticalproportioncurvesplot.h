#ifndef VERTICALPROPORTIONCURVESPLOT_H
#define VERTICALPROPORTIONCURVESPLOT_H

#include <qwt_plot.h>

class QwtPlotCurve;
class QwtPlotIntervalCurve;

class VerticalProportionCurvesPlot : public QwtPlot
{

    Q_OBJECT

public:

    VerticalProportionCurvesPlot( QWidget *parent = nullptr );

    /**
     * Sets the number of curves used to divide the plot canvas.
     * For example, if you have three facies, you need two curves to divide the
     * plot area into three proportion ranges.
     */
    void setNumberOfCurves( size_t number );

public slots:
    void insertCurve( int axis, double base );

private:

    void insertCurve(Qt::Orientation o, const QColor &c, double base , QString label);

    /** Removes all curves currently in the plot. */
    void clearCurves();

    /** Removes the filled areas between the calibration curves. */
    void clearFillAreas();

    /** The number of curves desired by the user. */
    size_t m_nCurves;

    /** The number of points in the curves. */
    size_t m_nPoints;

    /** The curves used to set proportion values. */
    std::vector<QwtPlotCurve*> m_curves;

    /** The filled areas between the calibration curves, normally used to calibrate
     * soft indicators for categorical variables. */
    std::vector<QwtPlotIntervalCurve*> m_fillAreas;

    /** The size of the curve handles in pixels. */
    size_t m_handleSize;

    /** The size of the legend icons in pixels. */
    size_t m_legendIconSize;
};

#endif // VERTICALPROPORTIONCURVESPLOT_H
