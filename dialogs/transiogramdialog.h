#ifndef TRANSIOGRAMDIALOG_H
#define TRANSIOGRAMDIALOG_H

#include <QDialog>

namespace Ui {
class TransiogramDialog;
}

namespace QtCharts{
    class QValueAxis;
    class QLineSeries;
}

class Attribute;
class FileSelectorWidget;
class VerticalTransiogramModel;

class TransiogramDialog : public QDialog
{
    Q_OBJECT

public:
    /** If vtm is null, a new Vertical Transiogram Model will be created and added to the Project. */
    explicit TransiogramDialog( VerticalTransiogramModel* vtm = nullptr,
                                QWidget *parent = nullptr );
    ~TransiogramDialog();

    void dragEnterEvent(QDragEnterEvent *e);

    void dragMoveEvent(QDragMoveEvent *e);

    void dropEvent(QDropEvent *e);

private:
    Ui::TransiogramDialog *ui;
    std::vector<Attribute*> m_categoricalAttributes;
    std::vector<QWidget *> m_transiogramChartViews; //all the transiograms of the matrix, filled row-wise first.
    std::vector<QWidget *> m_sumChartViews; //one per row of transiograms
    QString m_lastNameForSaving;

    int m_vSizePerTransiogram;
    int m_hSizePerTransiogram;

    VerticalTransiogramModel* m_vtm;

    QString m_formatForLabelsInXAxis = "%0.3g";
    QString m_formatForLabelsInYAxis = "%0.1g";

    void tryToAddAttribute( Attribute* attribute );
    void clearCharts();

    /** This method is called to create the charts for model review (no experimental transiography). */
    void makeChartsForModelReview();

private Q_SLOTS:
    void onResetAttributesList();
    void performCalculation();
    void onSave();
    void onTransiogramModelUpdated();
    void onDynamicFRD();
    void onCaptureExperimentalTransiography();
    void onZoomIn();
    void onZoomOut();
    void onCaptureExperimentalTransiographyWithoutOffscreen();
};

#endif // TRANSIOGRAMDIALOG_H
