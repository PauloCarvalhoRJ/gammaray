#ifndef MCRFBAYESIANSIMDIALOG_H
#define MCRFBAYESIANSIMDIALOG_H

#include <QDialog>

namespace Ui {
class MCRFBayesianSimDialog;
}

class Attribute;
class FileSelectorWidget;
class VariableSelector;
class CartesianGridSelector;
class ListBuilder;
class VariableListBuilder;
class CommonSimulationParameters;

/** The Markov Chains Random Field Simulation Dialog for a Bayesian approach (account for hyperparameter uncertainty). */
class MCRFBayesianSimDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MCRFBayesianSimDialog(QWidget *parent = nullptr);
    ~MCRFBayesianSimDialog();


private:
    Ui::MCRFBayesianSimDialog *ui;

    //------- pointers managed by Qt --------------
    FileSelectorWidget* m_primFileSelector;
    VariableSelector* m_primVarSelector;
    ListBuilder* m_primGradationValueList;
    CartesianGridSelector* m_simGridSelector;
    FileSelectorWidget* m_verticalTransiogramSelector;
    FileSelectorWidget* m_globalPDFSelector;
    ListBuilder* m_gradationalFieldVarList;
    std::vector< VariableListBuilder* > m_probFieldsListsBuilders;
    //----------------------------------------------

    CommonSimulationParameters* m_commonSimulationParameters;


private Q_SLOTS:
    void onRemakeProbabilityFieldsListsBuilders();
    void onPrimaryVariableChanged();
    void onCommonSimParams();
    void onRun();
};

#endif // MCRFBAYESIANSIMDIALOG_H
