#ifndef WIDGETGSLIBPARRANGE_H
#define WIDGETGSLIBPARRANGE_H

#include <QWidget>

class GSLibParRange;

namespace Ui {
class WidgetGSLibParRange;
}

class WidgetGSLibParRange : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetGSLibParRange(QWidget *parent = 0);
    ~WidgetGSLibParRange();
    void fillFields(GSLibParRange *param);
    void updateValue(GSLibParRange *param);
private:
    Ui::WidgetGSLibParRange *ui;
    //TODO: consider using another input widget for ranges without the limitation
    //      to integers imposed by QSlider
    int resolution; // values * 10 (see initializer, resolution is limited to one decimal place)
};

#endif // WIDGETGSLIBPARRANGE_H
