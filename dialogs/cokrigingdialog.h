#ifndef COKRIGINGDIALOG_H
#define COKRIGINGDIALOG_H

#include <QDialog>
#include <QVector>

namespace Ui {
class CokrigingDialog;
}

class PointSetSelector;
class VariableSelector;
class CartesianGridSelector;

class CokrigingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CokrigingDialog(QWidget *parent = 0);
    ~CokrigingDialog();

private:
    Ui::CokrigingDialog *ui;
    PointSetSelector* m_psInputSelector;
    VariableSelector* m_inputPrimVarSelector;
    QVector<VariableSelector*> m_inputSecVarsSelectors;
    CartesianGridSelector* m_cgEstimationGridSelector;
    CartesianGridSelector* m_cgSecondaryGridSelector;
    QVector<VariableSelector*> m_inputGridSecVarsSelectors;

private slots:
    void onNumberOfSecondaryVariablesChanged( int n );
    void onUpdateVariogramMatrix( int numberOfSecondaryVariables );
};

#endif // COKRIGINGDIALOG_H
