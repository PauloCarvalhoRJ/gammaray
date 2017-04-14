#ifndef SOFTINDICATORCALIBPLOT_H
#define SOFTINDICATORCALIBPLOT_H

#include <qwt_plot.h>

class SoftIndicatorCalibPlot : public QwtPlot
{
    Q_OBJECT
public:
    explicit SoftIndicatorCalibPlot( QWidget *parent = 0 );

    virtual bool eventFilter(QObject *object, QEvent * e);

public slots:
    void insertCurve( int axis, double base );

private:
    void insertCurve( Qt::Orientation o, const QColor &c, double base );
};

#endif // SOFTINDICATORCALIBPLOT_H
