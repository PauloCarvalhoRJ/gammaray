#ifndef SVDFACTORSSELECTIONCHARTVIEW_H
#define SVDFACTORSSELECTIONCHARTVIEW_H

#include <QChartView>

class QGraphicsSimpleTextItem;

class SVDFactorsSelectionChartCallout;

namespace QtCharts {
	class QLineSeries;
}

class SVDFactorsSelectionChartView : public QtCharts::QChartView
{
	Q_OBJECT
public:
	explicit SVDFactorsSelectionChartView(QtCharts::QChart* chart, QtCharts::QLineSeries* series, QWidget *parent = 0);

	virtual void resizeEvent(QResizeEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);

signals:

public slots:

private:
	QGraphicsSimpleTextItem* m_coordX;
	QGraphicsSimpleTextItem* m_coordY;
	QtCharts::QChart* m_chart;
	SVDFactorsSelectionChartCallout *m_tooltip;
	QList<SVDFactorsSelectionChartCallout *> m_callouts;

private slots:
	void keepCallout();
	void tooltip(QPointF point, bool state);
};

#endif // SVDFACTORSSELECTIONCHARTVIEW_H
