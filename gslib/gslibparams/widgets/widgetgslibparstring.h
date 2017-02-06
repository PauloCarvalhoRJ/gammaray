#ifndef WIDGETGSLIBPARSTRING_H
#define WIDGETGSLIBPARSTRING_H

#include <QWidget>

class GSLibParString;

namespace Ui {
class WidgetGSLibParString;
}

class WidgetGSLibParString : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetGSLibParString(QWidget *parent = 0);
    ~WidgetGSLibParString();
    void fillFields(GSLibParString* param);
    void fillFields(GSLibParString* param, const QString label);
    void updateValue(GSLibParString* param);
private:
    Ui::WidgetGSLibParString *ui;
};

#endif // WIDGETGSLIBPARSTRING_H
