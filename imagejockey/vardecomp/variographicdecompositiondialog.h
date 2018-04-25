#ifndef VARIOGRAPHICDECOMPOSITIONDIALOG_H
#define VARIOGRAPHICDECOMPOSITIONDIALOG_H

#include <QDialog>

namespace Ui {
class VariographicDecompositionDialog;
}

class VariographicDecompositionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VariographicDecompositionDialog(QWidget *parent = 0);
    ~VariographicDecompositionDialog();

private:
    Ui::VariographicDecompositionDialog *ui;
};

#endif // VARIOGRAPHICDECOMPOSITIONDIALOG_H
