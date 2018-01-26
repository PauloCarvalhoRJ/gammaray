#ifndef SVDFACTORSSELECTIONCHARTCALLOUT_H
#define SVDFACTORSSELECTIONCHARTCALLOUT_H

#include <QtCharts/QChartGlobal>
#include <QtWidgets/QGraphicsItem>
#include <QtGui/QFont>

class QGraphicsSceneMouseEvent;

namespace QtCharts{
	class QChart;
}

class SVDFactorsSelectionChartCallout : public QGraphicsItem
{
public:
	SVDFactorsSelectionChartCallout(QtCharts::QChart *chart);

	void setText(const QString &text);
	void setAnchor(QPointF point);
	void updateGeometry();

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

private:
	QString m_text;
	QRectF m_textRect;
	QRectF m_rect;
	QPointF m_anchor;
	QFont m_font;
	QtCharts::QChart *m_chart;
};

#endif // SVDFACTORSSELECTIONCHARTCALLOUT_H
