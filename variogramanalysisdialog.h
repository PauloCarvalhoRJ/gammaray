#ifndef VARIOGRAMANALYSISDIALOG_H
#define VARIOGRAMANALYSISDIALOG_H

#include <QDialog>

namespace Ui {
class VariogramAnalysisDialog;
}

class Attribute;
class GSLibParameterFile;
class CartesianGrid;
class ExperimentalVariogram;

class VariogramAnalysisDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * This constructor sets the dialog into full variographic analysis mode.
     * @brief VariogramAnalysisDialog
     * @param head  The variable for location u in the covariance computation.
     * @param tail  The variable for location u+h in the covariance computation.  If this is different from head, then
     *              a cross variogram analysis will be performed.
     */
    explicit VariogramAnalysisDialog(Attribute* head, Attribute* tail, QWidget *parent = 0);

    /**
     * This constructor sets the dialog into experimental variogram fitting.
     */
    explicit VariogramAnalysisDialog( ExperimentalVariogram* ev, QWidget *parent = 0 );

    ~VariogramAnalysisDialog();

private:
    Ui::VariogramAnalysisDialog *ui;
    Attribute* m_head;
    Attribute* m_tail;
    ExperimentalVariogram* m_ev;
    GSLibParameterFile* m_gpf_varmap;
    GSLibParameterFile* m_gpf_pixelplt;
    GSLibParameterFile* m_gpf_gamv;
    GSLibParameterFile* m_gpf_vargplt_exp_irreg;
    GSLibParameterFile* m_gpf_gam;
    CartesianGrid* m_varmap_grid;
    GSLibParameterFile* m_gpf_vmodel;

private slots:
    void onOpenVarMapParameters();
    void onOpenVarMapPlot();
    void onOpenExperimentalVariogramParamaters();
    void onOpenExperimentalVariogramPlot();
    void onSaveVarmapGrid();
    void onSaveExpVariogram();
    void onOpenVariogramModelParamateres();
    void onVmodelCompletion();
    void onSaveVariogramModel();
    // the slots below are called indirectly.
    void onGamv();
    void onGamvCompletion();
    void onVarmapCompletion();
    void onVargpltExperimentalIrregular();
    void onVargpltExperimentalRegular();
    /** @par4 path_to_vmodel_output If empty, only the experimental variogram is plotted. */
    void onVargplt(const QString path_to_exp_variogram_data, const QString path_to_vmodel_output);
    void onGam();
};

#endif // VARIOGRAMANALYSISDIALOG_H
