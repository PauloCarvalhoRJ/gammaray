#ifndef GSLIBPARAMETERSDIALOG_H
#define GSLIBPARAMETERSDIALOG_H

#include <QDialog>

namespace Ui {
class GSLibParametersDialog;
}

class GSLibParameterFile;

class GSLibParametersDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief The GSLibParametersDialog class allows one to edit a GSLib parameter object via GUI.
     * @param gpf Pointer to the GSLib parameter file object to edit.
     * @param parent
     */
    explicit GSLibParametersDialog(GSLibParameterFile *gpf, QWidget *parent = 0);
    ~GSLibParametersDialog();

private:
    Ui::GSLibParametersDialog *ui;
    GSLibParameterFile *_gpf;

    /** Internal method called to build the GUI according to the GSLibParameterFile object
     * passed in the constructor.
     */
    void addParamWidgets();

private slots:
    void onDialogAccepted();
    /**
     *  This slot is called when an WigetGSLibParUint widget's value changed.
     *  Not all of such widgets trigger this slot.  It usually happens for
     *  widgets corresponding to named parameters.  Also, there is no guarantee
     *  the parameter is named.  Client code must check whether parameter_name
     *  is not empty.  parameter_name is the paramter name declared in the GSLib
     *  parameter file template.
     */
    void someUintWidgetValueChanged( uint value, QString parameter_name );
};

#endif // GSLIBPARAMETERSDIALOG_H
