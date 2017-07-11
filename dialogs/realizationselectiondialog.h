#ifndef REALIZATIONSELECTIONDIALOG_H
#define REALIZATIONSELECTIONDIALOG_H

#include <QDialog>

namespace Ui {
class RealizationSelectionDialog;
}

class RealizationSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RealizationSelectionDialog(QWidget *parent = 0);
    ~RealizationSelectionDialog();

    void setNReals( int count );

    std::vector<int> getSelectedRealizations();

private:
    Ui::RealizationSelectionDialog *ui;
};

#endif // REALIZATIONSELECTIONDIALOG_H
