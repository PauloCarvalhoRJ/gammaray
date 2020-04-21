#ifndef VERTICALPROPORTIONCURVESPLOT_H
#define VERTICALPROPORTIONCURVESPLOT_H

#include <qwt_plot.h>

class VerticalProportionCurvesPlot : public QwtPlot
{
    Q_OBJECT
public:
    VerticalProportionCurvesPlot( QWidget *parent = nullptr );

    /** The number of curves desired by the user. */
    size_t m_nCurves;
};

#endif // VERTICALPROPORTIONCURVESPLOT_H
