#ifndef SOFTINDICATORCALIBPLOT_H
#define SOFTINDICATORCALIBPLOT_H

#include <qwt_plot.h>

class QwtPlotCurve;

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

public slots:
    void insertCurve( int axis, double base );

private:
    void insertCurve( Qt::Orientation o, const QColor &c, double base );

    /** Removes all curves currently in the plot. */
    void clearCurves();

    /** The data values. */
    std::vector<double> m_data;

    /** The curves used to calibrate the data into soft indicator values. */
    std::vector<QwtPlotCurve*> m_curves;

    /** Returns the highest value in the m_data vector or 100.0 if the vector is empty. */
    double getDataMax();

    /** Returns the lowest value in the m_data vector or 0.0 if the vector is empty. */
    double getDataMin();
};

#endif // SOFTINDICATORCALIBPLOT_H
