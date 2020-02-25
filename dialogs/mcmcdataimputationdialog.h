#ifndef MCMCDATAIMPUTATIONDIALOG_H
#define MCMCDATAIMPUTATIONDIALOG_H

#include <QDialog>

class FileSelectorWidget;
class VariableSelector;
class UnivariateDistributionSelector;

namespace Ui {
class MCMCDataImputationDialog;
}

class MCMCDataImputationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MCMCDataImputationDialog(QWidget *parent = nullptr);
    ~MCMCDataImputationDialog();

private:
    Ui::MCMCDataImputationDialog *ui;

    //------- pointers managed by Qt --------------
    FileSelectorWidget* m_fileSelector;
    VariableSelector* m_varSelector;
    FileSelectorWidget* m_ftmSelector;
    std::vector< UnivariateDistributionSelector* > m_distributionSelectors;
    VariableSelector* m_groupByVariableSelector;
    FileSelectorWidget* m_PDFSelector;
    //----------------------------------------------

private Q_SLOTS:
    void onRunMCMC();
    void onVariableChanged();
    void onRemakeDistributionCombos();
};

#endif // MCMCDATAIMPUTATIONDIALOG_H
