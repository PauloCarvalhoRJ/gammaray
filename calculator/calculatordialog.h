#ifndef CALCULATORDIALOG_H
#define CALCULATORDIALOG_H

#include <QDialog>
#include <QListWidgetItem>

namespace Ui {
class CalculatorDialog;
}

class ICalcPropertyCollection;

class CalculatorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CalculatorDialog(ICalcPropertyCollection *propertyCollection, QWidget *parent = 0);
    ~CalculatorDialog();

private:
    Ui::CalculatorDialog *ui;
    ICalcPropertyCollection *m_propertyCollection;

private slots:
    void onPropertyDoubleClicked(QListWidgetItem*item);
    void onSyntaxPage();
};

#endif // CALCULATORDIALOG_H
