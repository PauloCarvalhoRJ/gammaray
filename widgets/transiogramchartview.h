#ifndef TRANSIOGRAMCHARTVIEW_H
#define TRANSIOGRAMCHARTVIEW_H

#include "domain/verticaltransiogrammodel.h"

#include <QChartView>

class QGraphicsSimpleTextItem;

namespace QtCharts {
    class QValueAxis;
    class QLineSeries;
}

enum class TransiogramType : int {
    AUTO_TRANSIOGRAM,
    CROSS_TRANSIOGRAM
};

class TransiogramChartView : public QtCharts::QChartView
{
public:
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

private:
    QtCharts::QChart* m_chart;

    // QWidget interface
protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);

private:
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
    void showOrHideCrossHairs();
    void updateModelSeries();
};

#endif // TRANSIOGRAMCHARTVIEW_H
