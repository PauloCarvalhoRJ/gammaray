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
class FileSelectorWidget;
class DataFile;

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
	FileSelectorWidget* m_dataSetSelector;
	VariableSelector* m_DataSetVariableSelector;
    CartesianGrid* m_cg_estimation;
	GSLibParameterFile* m_gpfFK;
    void preview();
    void doFK();

private slots:
    void onParameters();
    void onSave( bool estimates = true );
    void onSaveEstimates();
    void onSaveOrUpdateVModel();
    void onKt3dCompletes();
    void onVariogramChanged();
	void onDataSetSelected(DataFile* dataFile);
};

#endif // FACTORIALKRIGINGDIALOG_H
