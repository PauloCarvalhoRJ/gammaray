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

class TransiogramDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TransiogramDialog( QWidget *parent = nullptr );
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

    void tryToAddAttribute( Attribute* attribute );
    void clearCharts();

private Q_SLOTS:
    void onResetAttributesList();
    void performCalculation();
    void onSave();
    void onTransiogramModelUpdated();
};

#endif // TRANSIOGRAMDIALOG_H
