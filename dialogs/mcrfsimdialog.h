#ifndef MCRFSIMDIALOG_H
#define MCRFSIMDIALOG_H

#include <QDialog>

namespace Ui {
class MCRFSimDialog;
}

class Attribute;
class FileSelectorWidget;
class VariableSelector;
class CartesianGridSelector;

/** The Markov Chains Random Field Simulation Dialog. */
class MCRFSimDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MCRFSimDialog(QWidget *parent = nullptr);
    ~MCRFSimDialog();


private:
    Ui::MCRFSimDialog *ui;

    FileSelectorWidget* m_primFileSelector;
    VariableSelector* m_primVarSelector;
    CartesianGridSelector* m_simGridSelector;
    FileSelectorWidget* m_verticalTransiogramSelector;
    FileSelectorWidget* m_globalPDFSelector;
    VariableSelector* m_gradationalFieldVarSelector;
    std::vector< VariableSelector* > m_probFieldsSelectors;

    void onRemakeProbabilityFieldsCombos();

private Q_SLOTS:
    void onCmbLateralGradationChanged();
    void onPrimaryVariableChanged();
};

#endif // MCRFSIMDIALOG_H
