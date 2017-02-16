#ifndef TRIADSEDITORDIALOG_H
#define TRIADSEDITORDIALOG_H

#include <QDialog>

namespace Ui {
class TriadsEditorDialog;
}

class File;

class TriadsEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TriadsEditorDialog(File* triadsFile, QWidget *parent = 0);
    ~TriadsEditorDialog();

    /** Displays an OK button, so the dialog closes with the QDialog::accept() slot, triggering
     * the QDialog::accepted() signal.  This is useful to trigger actions upon dialog closing. */
    void showOKbutton();

private:
    Ui::TriadsEditorDialog *ui;
    QList<QWidget*> m_tripletWidgets;
    File* m_triadsFile;

private slots:
    void onAddTriplet();
    void onRemoveTriplet();
    void onSave();
};

#endif // TRIADSEDITORDIALOG_H
