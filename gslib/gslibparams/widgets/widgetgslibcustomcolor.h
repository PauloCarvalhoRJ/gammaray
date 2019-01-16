#ifndef WIDGETGSLIBCUSTOMCOLOR_H
#define WIDGETGSLIBCUSTOMCOLOR_H

#include <QWidget>

namespace Ui {
class WidgetGSLibCustomColor;
}

class WidgetGSLibCustomColor : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetGSLibCustomColor(QWidget *parent = nullptr);
    ~WidgetGSLibCustomColor();

private:
    Ui::WidgetGSLibCustomColor *ui;
};

#endif // WIDGETGSLIBCUSTOMCOLOR_H
