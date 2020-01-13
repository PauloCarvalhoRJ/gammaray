#ifndef CHOOSEVARIABLEDIALOG_H
#define CHOOSEVARIABLEDIALOG_H

#include <QDialog>

namespace Ui {
class ChooseVariableDialog;
}

class DataFile;
class VariableSelector;

class ChooseVariableDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChooseVariableDialog(DataFile* df, const QString title, const QString caption, QWidget *parent = nullptr);
    ~ChooseVariableDialog();

    /** Returns -1 if the user opts to "NOT SET". */
    int getSelectedVariableIndex();

private:
    Ui::ChooseVariableDialog *ui;
    DataFile* m_df;
    VariableSelector* m_variableSelector;
};

#endif // CHOOSEVARIABLEDIALOG_H
