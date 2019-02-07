#ifndef TRANSIOGRAMDIALOG_H
#define TRANSIOGRAMDIALOG_H

#include <QDialog>

namespace Ui {
class TransiogramDialog;
}

class FaciesTransitionMatrix;

class TransiogramDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TransiogramDialog(FaciesTransitionMatrix *ftm, QWidget *parent = nullptr);
    ~TransiogramDialog();

private:
    Ui::TransiogramDialog *ui;
    FaciesTransitionMatrix* m_faciesTransitionMatrix;
    void performCalculation();
};

#endif // TRANSIOGRAMDIALOG_H
