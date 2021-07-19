#ifndef SUBGRIDDIALOG_H
#define SUBGRIDDIALOG_H

#include <QDialog>

class CartesianGrid;

namespace Ui {
class SubgridDialog;
}

class SubgridDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SubgridDialog( CartesianGrid* cg, QWidget *parent = nullptr);
    ~SubgridDialog();

private:
    Ui::SubgridDialog *ui;

    CartesianGrid* m_cg;

private Q_SLOTS:

    void onSave();
};

#endif // SUBGRIDDIALOG_H
