#ifndef MACHINELEARNINGDIALOG_H
#define MACHINELEARNINGDIALOG_H

#include <QDialog>

namespace Ui {
class MachineLearningDialog;
}

class MachineLearningDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MachineLearningDialog(QWidget *parent = 0);
    ~MachineLearningDialog();

private:
    Ui::MachineLearningDialog *ui;
};

#endif // MACHINELEARNINGDIALOG_H
