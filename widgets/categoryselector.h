#ifndef CATEGORYSELECTOR_H
#define CATEGORYSELECTOR_H

#include <QWidget>

namespace Ui {
class CategorySelector;
}

class CategoryDefinition;

class CategorySelector : public QWidget
{
    Q_OBJECT

public:
    explicit CategorySelector(CategoryDefinition *cd, QWidget *parent = 0);
    ~CategorySelector();

private:
    Ui::CategorySelector *ui;
};

#endif // CATEGORYSELECTOR_H
