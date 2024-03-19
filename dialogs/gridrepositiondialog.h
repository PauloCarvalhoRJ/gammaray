#ifndef GRIDREPOSITIONDIALOG_H
#define GRIDREPOSITIONDIALOG_H

#include <QDialog>

class CartesianGrid;

namespace Ui {
class GridRepositionDialog;
}

class GridRepositionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GridRepositionDialog(CartesianGrid* cg, QWidget *parent = nullptr);
    ~GridRepositionDialog();

public Q_SLOTS:
    void accept() override;

private:
    Ui::GridRepositionDialog *ui;

    CartesianGrid* m_cg;
};

#endif // GRIDREPOSITIONDIALOG_H
