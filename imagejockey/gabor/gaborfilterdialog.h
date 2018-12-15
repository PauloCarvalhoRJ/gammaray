#ifndef GABORFILTERDIALOG_H
#define GABORFILTERDIALOG_H

#include <QDialog>

namespace Ui {
class GaborFilterDialog;
}

class GaborFilterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GaborFilterDialog(QWidget *parent = 0);
    ~GaborFilterDialog();

private:
    Ui::GaborFilterDialog *ui;
};

#endif // GABORFILTERDIALOG_H
