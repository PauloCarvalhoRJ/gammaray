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
	CartesianGrid* m_cg_preview;
	CartesianGrid* m_cg_nSamples;
	GSLibParameterFile* m_gpfFK;
	QString m_varName;
	std::vector<double> m_results;
	void preview();
    void doFK();

private slots:
    void onParameters();
	void onSave( );
    void onSaveEstimates();
    void onVariogramChanged();
	void onDataSetSelected(DataFile* dataFile);
};

#endif // FACTORIALKRIGINGDIALOG_H
