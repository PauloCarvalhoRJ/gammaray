#ifndef CARTESIANGRIDDIALOG_H
#define CARTESIANGRIDDIALOG_H

#include <QDialog>

namespace Ui {
class CartesianGridDialog;
}

class CartesianGridDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CartesianGridDialog(QWidget *parent = 0, const QString file_path = "");
    ~CartesianGridDialog();

    double getX0();
    double getY0();
    double getZ0();
    double getDX();
    double getDY();
    double getDZ();
    uint getNX();
    uint getNY();
    uint getNZ();
    uint getNReal();
    double getRot();
    QString getNoDataValue();

private:
    Ui::CartesianGridDialog *ui;
    QString _file_path;
};

#endif // CARTESIANGRIDDIALOG_H
