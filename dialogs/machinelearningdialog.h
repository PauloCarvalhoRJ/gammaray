#ifndef MACHINELEARNINGDIALOG_H
#define MACHINELEARNINGDIALOG_H

#include <QDialog>

namespace Ui {
class MachineLearningDialog;
}

class FileSelectorWidget;
class VariableSelector;
class GSLibParameterFile;

class MachineLearningDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MachineLearningDialog(QWidget *parent = 0);
    ~MachineLearningDialog();

private:
    Ui::MachineLearningDialog *ui;
    FileSelectorWidget* m_trainingFileSelector;
    VariableSelector* m_trainingDependentVariableSelector;
    FileSelectorWidget* m_outputFileSelector;
    QVector<VariableSelector*> m_trainingVariableSelectors;
    QVector<VariableSelector*> m_outputVariableSelectors;

    VariableSelector *makeVariableSelector();

    bool getCARTParameters( GSLibParameterFile& gpf );
    void runCARTClassify();
    void runCARTRegression();
    bool getRandomForestParameters( GSLibParameterFile& gpf );
    void runRandomForestClassify();
    void runRandomForestRegression();

    bool isClassification();

    std::vector<int> getTrainingFeaturesIDList();
    std::vector<int> getOutputFeaturesIDList();

private slots:
    void runAlgorithm();
    void setupVariableSelectionWidgets( int numberOfVariables );
    void updateApplicationLabel( );
};

#endif // MACHINELEARNINGDIALOG_H
