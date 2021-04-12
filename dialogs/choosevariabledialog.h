#ifndef CHOOSEVARIABLEDIALOG_H
#define CHOOSEVARIABLEDIALOG_H

#include <QDialog>

namespace Ui {
class ChooseVariableDialog;
}

class DataFile;
class VariableSelector;

/** This is a dialog that allows the user to choose amongst the variables of a data set. */
class ChooseVariableDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief ChooseVariableDialog
     * @param df
     * @param title
     * @param caption
     * @param showNotSet If true (default), the user can opt to not set an attribute.
     * @param parent
     */
    explicit ChooseVariableDialog(DataFile* df,
                                  const QString title,
                                  const QString caption,
                                  bool showNotSet = true,
                                  QWidget *parent = nullptr);
    ~ChooseVariableDialog();

    /** Returns -1 if the user opts to "NOT SET". */
    int getSelectedVariableIndex();

private:
    Ui::ChooseVariableDialog *ui;
    DataFile* m_df;
    VariableSelector* m_variableSelector;
};

#endif // CHOOSEVARIABLEDIALOG_H
