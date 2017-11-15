#ifndef SGSIMDIALOG_H
#define SGSIMDIALOG_H

#include <QDialog>

namespace Ui {
class SGSIMDialog;
}

class GSLibParGrid;
class WidgetGSLibParGrid;
class PointSetSelector;
class VariableSelector;
class UnivariateDistributionSelector;
class DistributionFieldSelector;
class CartesianGridSelector;
class VariogramModelSelector;
class DataFile;
class GSLibParameterFile;
class VariogramModel;
class CartesianGrid;


class SGSIMDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SGSIMDialog( QWidget *parent = 0);
    ~SGSIMDialog();

private:
    Ui::SGSIMDialog *ui;
    GSLibParGrid* m_par;
    WidgetGSLibParGrid* m_gridParameters;
    PointSetSelector *m_primVarPSetSelector;
    VariableSelector *m_primVarSelector;
    VariableSelector *m_primVarWgtSelector;
    VariableSelector *m_primVarSecVarSelector;
    UnivariateDistributionSelector *m_refDistFileSelector;
    DistributionFieldSelector *m_refDistValuesSelector;
    DistributionFieldSelector *m_refDistFreqSelector;
    CartesianGridSelector *m_gridCopySpecsSelector;
    CartesianGridSelector *m_secVarGridSelector;
    VariableSelector *m_secVarVariableSelector;
    VariogramModelSelector *m_vModelSelector;
    GSLibParameterFile* m_gpf_sgsim;
    CartesianGrid* m_cg_simulation;
    GSLibParameterFile* m_gpf_gam;
    /** Called when the user changes the variogram model, so the variogram parameters
     * in m_gpf_kt3d are read from the newly selected variogram model.*/
    void updateVariogramParameters(VariogramModel *vm );
    void preview();

private slots:
    void onGridCopySpectsSelected( DataFile* grid );
    void onConfigAndRun();
    void onVariogramChanged();
    void onSgsimCompletes();
    void onRealizationHistogram();
    void onEnsembleHistogram();
    void onEnsembleVariogram();
    void onSaveEnsemble();
};

#endif // SGSIMDIALOG_H
