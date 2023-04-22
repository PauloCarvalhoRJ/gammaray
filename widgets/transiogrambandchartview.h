#ifndef TRANSIOGRAMBANDCHARTVIEW_H
#define TRANSIOGRAMBANDCHARTVIEW_H

#include "widgets/transiogramchartview.h"

/** Enum use to select which transiogram model of the band to edit/select.
 */
enum class WhichTransiogramModelOfTheBand : unsigned int {
    FIRST = 0,
    SECOND = 1
};

/** Does the same as TransiogramChartView, but allows to define
 * a second transiogram model corve to allow defining a band.
 */
class TransiogramBandChartView : public TransiogramChartView
{

    Q_OBJECT

public:
    /** The constructio.  Refer to TransiogramChartView documentation.*/
    TransiogramBandChartView(QtCharts::QChart* chart,
                             TransiogramType type,
                             double hMax,
                             QtCharts::QValueAxis *axisX,
                             QtCharts::QValueAxis *axisY,
                             QString headFaciesName,
                             QString tailFaciesName,
                             QWidget *parent = nullptr);

    void setModel2Parameters( double range2, double sill2 );

    VTransiogramStructureType getTransiogram2StructureType() const { return VTransiogramStructureType::SPHERIC; } //currently only spheric is supported
    VTransiogramRange getRange2() const { return m_range2; }
    VTransiogramSill getSill2() const { return m_sill2; }

    /** Returns the values that make up the 2nd transiogram model curve
     * This is useful to display the same curve in some other QChartView widget. */
    QtCharts::QLineSeries *getSeriesTransiogramModel2() const;

    void setModel2Visible( bool value );

Q_SIGNALS:
    /** Listening clients must use the get*() methods to retrieve the updated 2nd model parameters. */
    void model2WasUpdated();

    // QWidget interface
protected:
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);

protected:

    ////---------------- 2nd transiogram model parameters-----------
    /// currently only spheric type is supported
    double m_range2;
    double m_sill2;
    ////-------------------------------------------------------------

    WhichTransiogramModelOfTheBand m_whichModel;

    QtCharts::QLineSeries *m_seriesTransiogramModel2;
    bool m_modelVisible2;
    void updateModelSeries2();
};

#endif // TRANSIOGRAMBANDCHARTVIEW_H
