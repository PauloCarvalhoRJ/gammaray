#ifndef KRIGINGDIALOG_H
#define KRIGINGDIALOG_H

#include <QDialog>

namespace Ui {
class KrigingDialog;
}

class VariogramModelSelector;
class CartesianGridSelector;
class PointSetSelector;
class VariableSelector;
class GSLibParameterFile;
class VariogramModel;
class CartesianGrid;

class KrigingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KrigingDialog(QWidget *parent = 0);
    ~KrigingDialog();

private:
    Ui::KrigingDialog *ui;
    VariogramModelSelector* m_vModelSelector;
    CartesianGridSelector* m_cgSelector;
    CartesianGridSelector* m_cgSelectorSecondary;
    VariableSelector* m_cgSecondaryVariableSelector;
    PointSetSelector* m_psSelector;
    VariableSelector* m_PointSetVariableSelector;
    VariableSelector* m_PointSetSecondaryVariableSelector;
    GSLibParameterFile* m_gpf_kt3d;
    CartesianGrid* m_cg_estimation;
    void preview();
    /** Called when the user changes the variogram model, so the variogram parameters
     * in m_gpf_kt3d are read from the newly selected variogram model.*/
    void updateVariogramParameters( VariogramModel* vm );

private slots:
    void onParameters();
    void onXValidation();
    void onSave( bool estimates = true );
    void onSaveEstimates();
    void onSaveKVariances();
    void onSaveOrUpdateVModel();
    void onKt3dCompletes();
    void onVariogramChanged();
};

#endif // KRIGINGDIALOG_H
