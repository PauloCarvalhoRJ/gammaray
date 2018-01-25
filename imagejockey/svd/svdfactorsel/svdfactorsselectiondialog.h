#ifndef SVDFactorsSelectionDialog_H
#define SVDFactorsSelectionDialog_H

#include <QDialog>

namespace Ui {
class SVDFactorsSelectionDialog;
}

class SVDFactorsSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SVDFactorsSelectionDialog(QWidget *parent = 0);
    ~SVDFactorsSelectionDialog();

private:
    Ui::SVDFactorsSelectionDialog *ui;
};

#endif // SVDFactorsSelectionDialog_H
