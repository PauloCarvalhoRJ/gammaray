#ifndef AUTOMATICVARFITEXPERIMENTSDIALOG_H
#define AUTOMATICVARFITEXPERIMENTSDIALOG_H

#include <QDialog>

namespace Ui {
class AutomaticVarFitExperimentsDialog;
}

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
    void setFromSpinBox( double min, double max, double increase );
    void setToSpinBox( double min, double max, double increase );
    void setStepsSpinBox( int min, int max, int increase );

    int getParameterIndex();
    double getFrom();
    double getTo();
    int getNumberOfSteps();

private:
    Ui::AutomaticVarFitExperimentsDialog *ui;
};

#endif // AUTOMATICVARFITEXPERIMENTSDIALOG_H
