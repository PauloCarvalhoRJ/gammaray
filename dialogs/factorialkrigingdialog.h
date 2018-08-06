#ifndef FACTORIALKRIGINGDIALOG_H
#define FACTORIALKRIGINGDIALOG_H

#include <QDialog>

namespace Ui {
class FactorialKrigingDialog;
}

class VariogramModelSelector;
class CartesianGridSelector;
class PointSetSelector;
class VariableSelector;
class GSLibParameterFile;
class VariogramModel;
class CartesianGrid;

class FactorialKrigingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FactorialKrigingDialog(QWidget *parent = 0);
    ~FactorialKrigingDialog();

private:
    Ui::FactorialKrigingDialog *ui;
    VariogramModelSelector* m_vModelSelector;
    CartesianGridSelector* m_cgSelector;
    CartesianGridSelector* m_cgSelectorSecondary;
    VariableSelector* m_cgSecondaryVariableSelector;
    PointSetSelector* m_psSelector;
    VariableSelector* m_PointSetVariableSelector;
    VariableSelector* m_PointSetSecondaryVariableSelector;
    CartesianGrid* m_cg_estimation;
    void preview();
    /** Called when the user changes the variogram model, so the variogram parameters
     * in m_gpf_kt3d are read from the newly selected variogram model.*/
    void updateVariogramParameters(VariogramModel *vm );

private slots:
    void onParameters();
    void onSave( bool estimates = true );
    void onSaveEstimates();
    void onSaveOrUpdateVModel();
    void onKt3dCompletes();
    void onVariogramChanged();
};

#endif // FACTORIALKRIGINGDIALOG_H
