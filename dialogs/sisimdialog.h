#ifndef SISIMDIALOG_H
#define SISIMDIALOG_H

#include <QDialog>
#include "indicatorkrigingdialog.h"

class GSLibParGrid;
class WidgetGSLibParGrid;
class DataFile;

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
    PointSetSelector* m_SoftPointSetSelector;
    QList<VariableSelector*> m_SoftIndicatorVariablesSelectors;
    FileSelectorWidget* m_DensityFunctionSelector;
    GSLibParGrid* m_parGrid;
    WidgetGSLibParGrid* m_gridParametersWidget;
    CartesianGridSelector* m_gridCopySpecsSelector;
    QList<VariogramModelSelector*> m_variogramSelectors;
    ///---------------data member variables--------------------
    IKVariableType m_varType;
    GSLibParameterFile* m_gpf_sisim;
    ///--------------private methods----------------------------
    void addVariogramSelector();

private Q_SLOTS:
    void onUpdateSoftIndicatorVariablesSelectors();
    void onUpdateVariogramSelectors();
    void onGridCopySpectsSelected( DataFile* grid );
    void onConfigureAndRun();
};

#endif // SISIMDIALOG_H
