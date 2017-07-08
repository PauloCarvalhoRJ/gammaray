#ifndef GRIDRESAMPLEDIALOG_H
#define GRIDRESAMPLEDIALOG_H

#include <QDialog>

class CartesianGrid;

namespace Ui {
class GridResampleDialog;
}

class GridResampleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GridResampleDialog(CartesianGrid* cg, QWidget *parent = 0);
    ~GridResampleDialog();

    int getIrate();
    int getJrate();
    int getKrate();

private:
    Ui::GridResampleDialog *ui;
    CartesianGrid* _cg;
};

#endif // GRIDRESAMPLEDIALOG_H
