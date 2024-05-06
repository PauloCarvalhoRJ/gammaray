#ifndef DRIFTANALYSISDIALOG_H
#define DRIFTANALYSISDIALOG_H

#include <QDialog>

class DataFile;
class Attribute;
class LineChartWidget;

class DriftAnalysis;

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
    QString m_lastNameForDriftVariable;

    /**
     * Runs the drift analysis algorithm passed as parameter and displays its results in the dialog's charts.
     * @param clear_curves If true, clears all current curves, otherwise the resulting curves pile up.
     */
    void performDriftAnalysis( DriftAnalysis& driftAnalysis, bool clear_curves = true );

    /**
     * Updates the label showing the trend model formula with the passed model parameters.
     */
    void displayParamaters(const Quad3DTrendModelFittingAuxDefs::Parameters& model_parameters);

private Q_SLOTS:
    void onRun();
    void onFitTrendModel();
    void onRunFitTrendModel();
    void onSaveNewVariableWithDriftModel();
    void onCopyTrendModelAsCalcScript();
    void onEditTrendModelParameters();
};

#endif // DRIFTANALYSISDIALOG_H
