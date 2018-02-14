#ifndef SVDPARAMETERSDIALOG_H
#define SVDPARAMETERSDIALOG_H

#include <QDialog>

namespace Ui {
class SVDParametersDialog;
}

class SVDParametersDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SVDParametersDialog(QWidget *parent = 0);
    ~SVDParametersDialog();

    long getNumberOfFactors();

private:
    Ui::SVDParametersDialog *ui;
};

#endif // SVDPARAMETERSDIALOG_H
