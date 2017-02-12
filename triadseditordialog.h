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
