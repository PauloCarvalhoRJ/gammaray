#ifndef CALCULATORDIALOG_H
#define CALCULATORDIALOG_H

#include <QDialog>

namespace Ui {
class CalculatorDialog;
}

class CalculatorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CalculatorDialog(QWidget *parent = 0);
    ~CalculatorDialog();

private:
    Ui::CalculatorDialog *ui;
};

#endif // CALCULATORDIALOG_H
