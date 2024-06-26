#ifndef TRANSIOGRAMCHARTVIEW_H
#define TRANSIOGRAMCHARTVIEW_H

#include "domain/verticaltransiogrammodel.h"

#include <QChartView>

class QGraphicsSimpleTextItem;

namespace QtCharts {
    class QValueAxis;
    class QLineSeries;
}

/**
 * This is a Qt ChartView specialized in showing and editing a
 * transiogram (experimental and model curve).
 */
class TransiogramChartView : public QtCharts::QChartView
{
    Q_OBJECT

public:
    /**
     * The constructor.
     * @param chart QChart object required to build any QChartView (see QCharts documentation).
     * @param type The type of transiogram this view (auto- or cross-transiogram - see "Transiograms for Characterizing Spatial Variability of Soil Classes", - Li, W. (2007)).
     * @param hMax The end-of-scale X axis value.
     * @param axisX Axis object for the X axis.  This usually allows for styling (e.g. ticks, labels, etc.).
     * @param axisY Axis object for the Y axis.  This usually allows for styling (e.g. ticks, labels, etc.).
     * @param headFaciesName The name of the Attribute that is the head variable (refer to geostatistics literarture).
     * @param tailFaciesName The name of the Attribute that is the tail variable (refer to geostatistics literarture).
     * @param parent Qt widget that serves as parent widget (see Qt documentation).
     */
    TransiogramChartView(QtCharts::QChart* chart,
                         TransiogramType type,
                         double hMax,
                         QtCharts::QValueAxis *axisX,
                         QtCharts::QValueAxis *axisY,
                         QString headFaciesName,
                         QString tailFaciesName,
                         QWidget *parent = nullptr);

    void setModelParameters( double range, double sill );

    VTransiogramStructureType getTransiogramStructureType() const { return VTransiogramStructureType::SPHERIC; } //currently only spheric is supported
    VTransiogramRange getRange() const { return m_range; }
    VTransiogramSill getSill() const { return m_sill; }

    QString getHeadFaciesName() const { return m_headFaciesName; }
    QString getTailFaciesName() const { return m_tailFaciesName; }

    /** Returns the values that make up the transiogram model curve
     * This is useful to display the same curve in some other QChartView widget. */
    QtCharts::QLineSeries *getSeriesTransiogramModel() const;

    void setModelVisible( bool value );

Q_SIGNALS:
    /** Listening clients must use the get*() methods to retrieve the updated model parameters. */
    void modelWasUpdated();

protected:
    QtCharts::QChart* m_chart;

    // QWidget interface
protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);

protected:
    QRubberBand* m_rubberBandH;
    QRubberBand* m_rubberBandV;
    ////---------------- the transiogram model parameters-----------
    /// currently only spheric type is supported
    double m_range;
    double m_sill;
    ////-------------------------------------------------------------
    TransiogramType m_type; // auto- or cross-transiogram
    QtCharts::QValueAxis *m_axisX;
    QtCharts::QValueAxis *m_axisY;
    double m_hMax; //maximum h
    QtCharts::QLineSeries *m_seriesTransiogramModel;
    bool m_mouseDown;
    QString m_headFaciesName;
    QString m_tailFaciesName;
    bool m_modelVisible;
    void showOrHideCrossHairs();
    void updateModelSeries();
    void updateCrossHairsPosition(QMouseEvent *event);
};

#endif // TRANSIOGRAMCHARTVIEW_H
