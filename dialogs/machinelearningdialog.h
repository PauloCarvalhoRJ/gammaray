#ifndef MACHINELEARNINGDIALOG_H
#define MACHINELEARNINGDIALOG_H

#include <QDialog>

namespace Ui {
class MachineLearningDialog;
}

class FileSelectorWidget;
class VariableSelector;

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

private slots:
    void runAlgorithm();
    void setupVariableSelectionWidgets( int numberOfVariables );
    void updateApplicationLabel( );
};

#endif // MACHINELEARNINGDIALOG_H
