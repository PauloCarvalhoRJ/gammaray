#ifndef SISIMDIALOG_H
#define SISIMDIALOG_H

#include <QDialog>
#include "indicatorkrigingdialog.h" //for IKVariableType enum

namespace Ui {
class SisimDialog;
}

class SisimDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SisimDialog(IKVariableType varType, QWidget *parent = 0);
    ~SisimDialog();

private:
    Ui::SisimDialog *ui;
};

#endif // SISIMDIALOG_H
