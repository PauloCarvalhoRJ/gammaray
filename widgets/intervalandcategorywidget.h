#ifndef INTERVALANDCATEGORYWIDGET_H
#define INTERVALANDCATEGORYWIDGET_H

#include <QWidget>

namespace Ui {
class IntervalAndCategoryWidget;
}

class CategoryDefinition;
class CategorySelector;

class IntervalAndCategoryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit IntervalAndCategoryWidget(CategoryDefinition* cd, QWidget *parent = 0);
    ~IntervalAndCategoryWidget();

    double getIntervalLow();
    double getIntervalHigh();
    uint getCategoryCode();

    void setIntervalLow( double value );
    void setIntervalHigh( double value );
    void setCategoryCode( uint value );

private:
    Ui::IntervalAndCategoryWidget *ui;
    CategorySelector* m_categorySelector;
};

#endif // INTERVALANDCATEGORYWIDGET_H
