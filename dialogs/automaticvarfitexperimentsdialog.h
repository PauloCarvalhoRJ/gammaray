#ifndef AUTOMATICVARFITEXPERIMENTSDIALOG_H
#define AUTOMATICVARFITEXPERIMENTSDIALOG_H

#include <QDialog>

namespace Ui {
class AutomaticVarFitExperimentsDialog;
}

typedef std::tuple<double, double, double> MinMaxIncreaseDouble;
typedef std::tuple<int, int, int> MinMaxIncreaseInt;

/** This dialog is called from AutomaticVarFitDialog to run experiments with the
 * parameters of the optimization algorithms.
 */
class AutomaticVarFitExperimentsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AutomaticVarFitExperimentsDialog(QWidget *parent = nullptr);
    ~AutomaticVarFitExperimentsDialog();

    void setAgorithmLabel( QString label );
    void setParameterList( QStringList names );
    void setFromSpinBoxConfigs(  const std::vector< MinMaxIncreaseDouble > oneConfigPerParameter );
    void setToSpinBoxConfigs(    const std::vector< MinMaxIncreaseDouble > oneConfigPerParameter );
    void setStepsSpinBoxConfigs( const std::vector< MinMaxIncreaseInt >    oneConfigPerParameter );

    int getParameterIndex();
    double getFrom();
    double getTo();
    int getNumberOfSteps();

protected:
    //QWidget interface
    void showEvent( QShowEvent* event );

private:
    Ui::AutomaticVarFitExperimentsDialog *ui;
    std::vector< MinMaxIncreaseDouble > m_configsForFromSpinBox;
    std::vector< MinMaxIncreaseDouble > m_configsForToSpinBox;
    std::vector< MinMaxIncreaseInt >    m_configsForStepsSpinBox;

private Q_SLOTS:
    void onUpdateSpinBoxes();
};

#endif // AUTOMATICVARFITEXPERIMENTSDIALOG_H
