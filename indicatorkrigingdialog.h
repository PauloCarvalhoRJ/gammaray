#ifndef INDICATORKRIGINGDIALOG_H
#define INDICATORKRIGINGDIALOG_H

#include <QDialog>
#include <QList>

namespace Ui {
class IndicatorKrigingDialog;
}

class VariogramModelSelector;
class CartesianGridSelector;
class VariableSelector;
class PointSetSelector;
class VariogramModelSelector;
class FileSelectorWidget;
class GSLibParameterFile;
class CartesianGrid;

/*! The variable type result in different indicator kriging beahvior. */
enum class IKVariableType : uint {
    CONTINUOUS = 0, /*!< Indicator kriging will treat the variable as continuous values. */
    CATEGORICAL     /*!< Indicator kriging will treat the variable as category IDs. */
};


class IndicatorKrigingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IndicatorKrigingDialog( IKVariableType varType, QWidget *parent = 0);
    ~IndicatorKrigingDialog();

private:
    Ui::IndicatorKrigingDialog *ui;
    GSLibParameterFile* m_gpf_ik3d;
    CartesianGridSelector* m_cgSelector;
    PointSetSelector* m_psSelector;
    VariableSelector* m_PointSetVariableSelector;
    PointSetSelector* m_psSoftSelector;
    QList<VariableSelector*> m_SoftIndicatorVariablesSelectors;
    FileSelectorWidget* m_dfSelector;
    QList<VariogramModelSelector*> m_variogramSelectors;
    void addVariogramSelector();
    IKVariableType m_varType;
    CartesianGrid* m_cg_estimation;
    void preview();

private slots:
    void onUpdateVariogramSelectors();
    void onConfigureAndRun();
    void onIk3dCompletes();
    void onUpdateSoftIndicatorVariablesSelectors();
};

#endif // INDICATORKRIGINGDIALOG_H
