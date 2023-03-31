#ifndef LISTBUILDERDIALOG_H
#define LISTBUILDERDIALOG_H

#include <QDialog>

class ListBuilder;

namespace Ui {
class ListBuilderDialog;
}

/**
 * This wraps a list builder widget in a dialog window with ok/cancel buttons.
 */
class ListBuilderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ListBuilderDialog(QWidget *parent = nullptr);
    ~ListBuilderDialog();


    /** Returns the pointer to the internal list builder widget. */
    ListBuilder* getListBuilder(){ return m_listBuilder; }

private:
    Ui::ListBuilderDialog *ui;

    ListBuilder* m_listBuilder;
};

#endif // LISTBUILDERDIALOG_H
