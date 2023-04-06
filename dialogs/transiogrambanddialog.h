#ifndef TRANSIOGRAMBANDDIALOG_H
#define TRANSIOGRAMBANDDIALOG_H

#include "dialogs/transiogramdialog.h"

/**
 * Does the same as TransiogramDialog but allows editing and reviewing a
 * second vertical transiogram model so the user can define a band instead
 * of single curve.
 */
class TransiogramBandDialog : public TransiogramDialog
{

    Q_OBJECT

public:
    /** If vtm and/or vtem2 is/are null, two new Vertical Transiogram Model will be created
     * and added to the Project. */
    TransiogramBandDialog( VerticalTransiogramModel* vtm = nullptr,
                           VerticalTransiogramModel* vtm2 = nullptr,
                           QWidget *parent = nullptr);
protected:
    /** The name for the second transiogram model file. */
    QString m_lastNameForSaving2;

    /** The second transiogram model to define the band. */
    VerticalTransiogramModel* m_vtm2;

    // TransiogramDialog interface
protected:
    virtual TransiogramChartView* makeNewTransiogramChartView( QtCharts::QChart* chart,
                                                               TransiogramType type,
                                                               double hMax,
                                                               QtCharts::QValueAxis *axisX,
                                                               QtCharts::QValueAxis *axisY,
                                                               QString headFaciesName,
                                                               QString tailFaciesName,
                                                               double initialRange,
                                                               double initialSill ) override;

protected Q_SLOTS:
    virtual void onSave() override;
    void onTransiogramModel2Updated();
};

#endif // TRANSIOGRAMBANDDIALOG_H
