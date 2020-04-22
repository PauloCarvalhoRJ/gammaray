#ifndef SOFTINDICATORCALIBPLOT_H
#define SOFTINDICATORCALIBPLOT_H

#include <qwt_plot.h>

class QwtPlotCurve;
class QwtPlotIntervalCurve;

/*! The variable type result in different soft indicators returned by getSoftIndicators(). */
enum class SoftIndicatorCalculationMode : uint {
    CONTINUOUS = 0, /*!< Soft indicators will be local threshold cumulative probabilities. */
    CATEGORICAL     /*!< Soft indicators will be local category probabilities. */
};


/**
 * The SoftIndicatorCalibPlot class is a widget that can be added to any Qt GUI to enable soft indicator calibration.
 */
class SoftIndicatorCalibPlot : public QwtPlot
{
    Q_OBJECT
public:
    explicit SoftIndicatorCalibPlot( QWidget *parent = 0 );

    virtual bool eventFilter(QObject *object, QEvent * e);

    /** Sets data values using move semantics, instead of a traditional element-by-element copy.
     * @note The source vector will be empty after this call. */
    void transferData( std::vector<double>& source );

    /**
     * Sets the number of curves used to divide the calibration canvas.
     * For example, if you have three facies (categorical variable), you need two curves.
     * If you have two thresholds (continuous variable), then you need two curves.
     */
    void setNumberOfCurves( size_t number );

    /**
     * Computes, for each datum in m_data, the soft indicators as a function of the curves set by the user.
     * If mode is CATEGORICAL, then soft indicators are computed as the ammount of probabilities between each curve
     * and between the curves and 0.0% and 100.0%.  So, if you set two curves, you get three soft probabilities per datum.
     * If mode is CONTINUOUS, then soft indicators are computed as the probability value at each curve
     * So, if you set two curves, you get two soft probabilities (cumulative) per datum.
     * The outer vector corresponds to each category/threshold and the inner vector corresponds to the soft indicators
     * computed for a category/threshold.
     */
    std::vector< std::vector< double > > getSoftIndicators( SoftIndicatorCalculationMode mode );

    /**
     * Sets the X-axis label, which is normally the variable name with the measurement unit between parentheses.
     */
    void setXAxisLabel( QString text );

    /**
     * Fills the plot area with the given color between the calibration curve given by base_curve index and the one
     * immediately above it.  If base_curve is -1 or less, the "base curve" will be the plot bottom side.
     * If base_curve is the index of the highest one or higher, the "top curve" will be the plot top side.
     * @param label Text that will appear in the plot legend (e.g. Sandstone).
     */
    void fillColor(const QColor &color, int base_curve , const QString label );

    /** Sets the legend label of a calibration curve given its index.  This is normally used when calibrating
     * soft probabilities (cumulative) for thresholds of a continuous variable.
     * Setting a label for a curve also enables its legend icon.
     */
    void setCurveLabel( int index, QString label );

    /** Sets the color of a calibration curve given its index.  This is normally used when calibrating
     * soft probabilities (cumulative) for thresholds of a continuous variable.
     * Setting a color for a curve also sets its legend icon color, if the curve has a label set.
     * By default, curves are rendered in black.
     */
    void setCurveColor( int index, QColor color );

    /** Sets the given curve base, resulting in a flat line at the given value.
      *  The value must be between 0.0 and 100.0 (will be truncated otherside).
      * Other curves will also be adjusted to prevent undue crossing.
      */
    void setCurveBase( int index, double value );

    /** Changes the geometry of the fill areas to follow the calibration curves.
     * Does nothing if there is no fill areas between the curves.
     */
    void updateFillAreas();

public slots:
    void insertCurve( int axis, double base );

private:
    void insertCurve(Qt::Orientation o, const QColor &c, double base , QString label);

    /** Removes all curves currently in the plot. */
    void clearCurves();

    /** Removes the filled areas between the calibration curves. */
    void clearFillAreas();

    /**
     *  Updates the curves other than the given one so they do not cross each other.
     */
    void pushCurves( QwtPlotCurve* curve );

    /** The data values. */
    std::vector<double> m_data;

    /** The curves used to calibrate the data into soft indicator values. */
    std::vector<QwtPlotCurve*> m_curves;

    /** The filled areas between the calibration curves, normally used to calibrate
     * soft indicators for categorical variables. */
    std::vector<QwtPlotIntervalCurve*> m_fillAreas;

    /** Returns the highest value in the m_data vector or 100.0 if the vector is empty. */
    double getDataMax();

    /** Returns the lowest value in the m_data vector or 0.0 if the vector is empty. */
    double getDataMin();

    /** The number of curves desired by the user. */
    size_t m_nCurves;

    /** The size of the curve handles in pixels. */
    size_t m_handleSize;

    /** The size of the legend icons in pixels. */
    size_t m_legendIconSize;

private slots:
    /** This must be triggered when a curve is changed. */
    void onCurveChanged( QwtPlotCurve* changed_curve );
};

#endif // SOFTINDICATORCALIBPLOT_H
