#ifndef WIDGETGSLIBPARDOUBLE_H
#define WIDGETGSLIBPARDOUBLE_H

#include <QWidget>

class GSLibParDouble;

namespace Ui {
class WidgetGSLibParDouble;
}

class WidgetGSLibParDouble : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetGSLibParDouble(QWidget *parent = 0);
    ~WidgetGSLibParDouble();

    void fillFields(double param, QString description);
    void updateValue( double *var );
    void fillFields( GSLibParDouble *param );
    void updateValue( GSLibParDouble *param );
private:
    Ui::WidgetGSLibParDouble *ui;
};

#endif // WIDGETGSLIBPARDOUBLE_H
