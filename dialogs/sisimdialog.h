#ifndef SISIMDIALOG_H
#define SISIMDIALOG_H

#include <QDialog>
#include "indicatorkrigingdialog.h"

class GSLibParGrid;
class WidgetGSLibParGrid;
class DataFile;
class DistributionFieldSelector;

namespace Ui {
class SisimDialog;
}

class SisimDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SisimDialog(IKVariableType varType, QWidget *parent = 0);
    ~SisimDialog();

private:
    ///--------------GUI member variables---------------------
    Ui::SisimDialog *ui;
    PointSetSelector* m_InputPointSetFileSelector;
    VariableSelector* m_InputVariableSelector;
    FileSelectorWidget* m_SoftDataSetSelector;
    QList<VariableSelector*> m_SoftIndicatorVariablesSelectors;
    FileSelectorWidget* m_DensityFunctionSelector;
    GSLibParGrid* m_parGrid;
    WidgetGSLibParGrid* m_gridParametersWidget;
    CartesianGridSelector* m_gridCopySpecsSelector;
    QList<VariogramModelSelector*> m_variogramSelectors;
	FileSelectorWidget* m_DensityFunctionSecondarySelector;
	FileSelectorWidget* m_BidistributionSelector;
	DistributionFieldSelector* m_BidistPrimaryValuesFieldSelector;
	DistributionFieldSelector* m_BidistSecondaryValuesFieldSelector;
	DistributionFieldSelector* m_BidistProbabilitiesFieldSelector;
	///---------------data member variables--------------------
    IKVariableType m_varType;
    GSLibParameterFile* m_gpf_sisim;
    CartesianGrid* m_cg_simulation;
    GSLibParameterFile* m_gpf_gam;
	GSLibParameterFile* m_gpf_postsim;
	CartesianGrid* m_cg_postsim;
    ///--------------private methods----------------------------
    void addVariogramSelector();
    void preview();
	void previewPostsim();
    void configureSoftDataUI();

private Q_SLOTS:
    void onUpdateSoftIndicatorVariablesSelectors();
    void onUpdateVariogramSelectors();
    void onGridCopySpectsSelected( DataFile* grid );
    void onConfigureAndRun();
    void onSisimCompletes();
    void onRealizationHistogram();
    void onEnsembleHistogram();
    void onEnsembleVariogram();
    void onSaveEnsemble();
	void onPostsim();
	void onSavePostsim();
    void onSisimProgramChanged(QString);
};

#endif // SISIMDIALOG_H
