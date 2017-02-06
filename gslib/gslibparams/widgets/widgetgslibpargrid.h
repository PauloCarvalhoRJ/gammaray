#ifndef WIDGETGSLIBPARGRID_H
#define WIDGETGSLIBPARGRID_H

#include <QWidget>

namespace Ui {
class WidgetGSLibParGrid;
}

class GSLibParGrid;

class WidgetGSLibParGrid : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetGSLibParGrid(QWidget *parent = 0);
    ~WidgetGSLibParGrid();

    void fillFields( GSLibParGrid* param );
    void updateValue( GSLibParGrid* param );

private:
    Ui::WidgetGSLibParGrid *ui;
    QList<QWidget*> _widgets;
};

#endif // WIDGETGSLIBPARGRID_H
