#ifndef MCMCDATAIMPUTATIONDIALOG_H
#define MCMCDATAIMPUTATIONDIALOG_H

#include <QDialog>

class FileSelectorWidget;
class VariableSelector;

namespace Ui {
class MCMCDataImputationDialog;
}

class MCMCDataImputationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MCMCDataImputationDialog(QWidget *parent = 0);
    ~MCMCDataImputationDialog();

private:
    Ui::MCMCDataImputationDialog *ui;

    //------- pointers managed by Qt --------------
    FileSelectorWidget* m_fileSelector;
    VariableSelector* m_varSelector;
    //----------------------------------------------
};

#endif // MCMCDATAIMPUTATIONDIALOG_H
