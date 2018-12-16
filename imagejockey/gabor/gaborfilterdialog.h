#ifndef GABORFILTERDIALOG_H
#define GABORFILTERDIALOG_H

#include <QDialog>

namespace Ui {
class GaborFilterDialog;
}

class IJAbstractCartesianGrid;

class GaborFilterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GaborFilterDialog(IJAbstractCartesianGrid* inputGrid,
                               uint inputVariableIndex,
                               QWidget *parent = 0);
    ~GaborFilterDialog();

private:
    Ui::GaborFilterDialog *ui;
    IJAbstractCartesianGrid* m_inputGrid;
    uint m_inputVariableIndex;

private Q_SLOTS:
    void onPerformGaborFilter();
};

#endif // GABORFILTERDIALOG_H
