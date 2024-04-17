#ifndef DRIFTANALYSISDIALOG_H
#define DRIFTANALYSISDIALOG_H

#include <QDialog>

class DataFile;
class Attribute;
class LineChartWidget;

namespace Ui {
class DriftAnalysisDialog;
}

class DriftAnalysisDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DriftAnalysisDialog(DataFile* dataFile, Attribute* attribute, QWidget *parent = nullptr);
    ~DriftAnalysisDialog();

private:
    Ui::DriftAnalysisDialog *ui;
    DataFile*  m_dataFile;
    Attribute* m_attribute;
    LineChartWidget* m_chartWestEast;
    LineChartWidget* m_chartSouthNorth;
    LineChartWidget* m_chartVertical;

private Q_SLOTS:
    void onRun();
};

#endif // DRIFTANALYSISDIALOG_H
