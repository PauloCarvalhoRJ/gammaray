#ifndef FACIESTRANSITIONMATRIXOPTIONSDIALOG_H
#define FACIESTRANSITIONMATRIXOPTIONSDIALOG_H

#include <QDialog>

namespace Ui {
class FaciesTransitionMatrixOptionsDialog;
}

class FaciesTransitionMatrixOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FaciesTransitionMatrixOptionsDialog(QWidget *parent = nullptr);
    ~FaciesTransitionMatrixOptionsDialog();

    bool isIgnoreGapsChecked();

private:
    Ui::FaciesTransitionMatrixOptionsDialog *ui;
};

#endif // FACIESTRANSITIONMATRIXOPTIONSDIALOG_H
