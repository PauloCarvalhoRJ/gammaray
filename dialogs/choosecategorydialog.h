#ifndef CHOOSECATEGORYDIALOG_H
#define CHOOSECATEGORYDIALOG_H

#include <QDialog>

namespace Ui {
class ChooseCategoryDialog;
}

class ChooseCategoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChooseCategoryDialog(QWidget *parent = nullptr);
    ~ChooseCategoryDialog();

private:
    Ui::ChooseCategoryDialog *ui;
};

#endif // CHOOSECATEGORYDIALOG_H
