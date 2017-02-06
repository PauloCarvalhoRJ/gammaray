#ifndef WIDGETGSLIBPARLIMITSDOUBLE_H
#define WIDGETGSLIBPARLIMITSDOUBLE_H

#include <QWidget>

class GSLibParLimitsDouble;
class WidgetGSLibParDouble;

namespace Ui {
class WidgetGSLibParLimitsDouble;
}

class WidgetGSLibParLimitsDouble : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetGSLibParLimitsDouble(QWidget *parent = 0);
    ~WidgetGSLibParLimitsDouble();
    void fillFields(GSLibParLimitsDouble* param);
    void updateValue( GSLibParLimitsDouble* param );
private:
    Ui::WidgetGSLibParLimitsDouble *ui;
    WidgetGSLibParDouble *_widgetMin;
    WidgetGSLibParDouble *_widgetMax;
};

#endif // WIDGETGSLIBPARLIMITSDOUBLE_H
