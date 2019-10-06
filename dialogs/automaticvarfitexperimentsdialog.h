#ifndef AUTOMATICVARFITEXPERIMENTSDIALOG_H
#define AUTOMATICVARFITEXPERIMENTSDIALOG_H

#include <QDialog>

namespace Ui {
class AutomaticVarFitExperimentsDialog;
}

typedef std::tuple<double, double, double> MinMaxIncreaseDouble;
typedef std::tuple<int, int, int> MinMaxIncreaseInt;

/** This dialog is called from AutomaticVarFitDialog to run experiments with the
 * parameters of the optimization algorithms.  This dialog serves to allow the user
 * to specify experimentation ranges for the algorithms' parameters.
 */
class AutomaticVarFitExperimentsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AutomaticVarFitExperimentsDialog(QWidget *parent = nullptr);
    ~AutomaticVarFitExperimentsDialog();

    /** Puts the name of the algorithm to be studied in the appropriate label.*/
    void setAgorithmLabel( QString label );

    /** Populates the combobox with the names of the parameters available to experimentation. */
    void setParameterList( QStringList names );

    /** Sets the possible configurations for the "from:" double spin box.
     * You must provide one triplet for each parameter passed in setParameterList().
     * Each triplet is a tuple consisting of min., max. and increase step values.
     */
    void setFromSpinBoxConfigs(  const std::vector< MinMaxIncreaseDouble > oneConfigPerParameter );

    /** Sets the possible configurations for the "to:" double spin box.
     * You must provide one triplet for each parameter passed in setParameterList().
     * Each triplet is a tuple consisting of min., max. and increase step values.
     */
    void setToSpinBoxConfigs(    const std::vector< MinMaxIncreaseDouble > oneConfigPerParameter );


    /** Sets the possible configurations for the "steps:" integer spin box.
     * You must provide one triplet for each parameter passed in setParameterList().
     * Each triplet is a tuple consisting of min., max. and increase step integer values.
     */
    void setStepsSpinBoxConfigs( const std::vector< MinMaxIncreaseInt >    oneConfigPerParameter );

    /** Returns the index of the parameter name selected by the user. */
    int getParameterIndex();

    /** Returns the "from:" value set by the user. */
    double getFrom();

    /** Returns the "to:" value set by the user. */
    double getTo();

    /** Returns the number of steps between "from:" and "to:" values set by the user. */
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
