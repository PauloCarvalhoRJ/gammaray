#ifndef CHOOSECATEGORYDIALOG_H
#define CHOOSECATEGORYDIALOG_H

#include <QDialog>

class CategoryDefinition;
class CategorySelector;

namespace Ui {
class ChooseCategoryDialog;
}

class ChooseCategoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChooseCategoryDialog( CategoryDefinition* cd, const QString title, const QString caption, QWidget *parent = nullptr);
    ~ChooseCategoryDialog();

    uint getSelectedCategoryCode();

private:
    Ui::ChooseCategoryDialog *ui;
    CategoryDefinition* m_cd;
    CategorySelector* m_cdSelector;
};

#endif // CHOOSECATEGORYDIALOG_H
