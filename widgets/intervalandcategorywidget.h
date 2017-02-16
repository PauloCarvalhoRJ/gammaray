#ifndef INTERVALANDCATEGORYWIDGET_H
#define INTERVALANDCATEGORYWIDGET_H

#include <QWidget>

namespace Ui {
class IntervalAndCategoryWidget;
}

class CategoryDefinition;

class IntervalAndCategoryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit IntervalAndCategoryWidget(CategoryDefinition* cd, QWidget *parent = 0);
    ~IntervalAndCategoryWidget();

private:
    Ui::IntervalAndCategoryWidget *ui;
};

#endif // INTERVALANDCATEGORYWIDGET_H
