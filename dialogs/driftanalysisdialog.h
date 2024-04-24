#ifndef DRIFTANALYSISDIALOG_H
#define DRIFTANALYSISDIALOG_H

#include <QDialog>

class DataFile;
class Attribute;
class LineChartWidget;

namespace Quad3DTrendModelFittingAuxDefs{
    class Parameters;
}

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
    std::unique_ptr<Quad3DTrendModelFittingAuxDefs::Parameters> m_lastFitDriftModelParameters;

private Q_SLOTS:
    void onRun();
    void onFitTrendModel();
    void onRunFitTrendModel();
    void onSaveNewVariableWithDriftModel();
};

#endif // DRIFTANALYSISDIALOG_H
