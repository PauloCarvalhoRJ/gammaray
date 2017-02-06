#ifndef WIDGETGSLIBPARCOLOR_H
#define WIDGETGSLIBPARCOLOR_H

#include <QWidget>

class GSLibParColor;

namespace Ui {
class WidgetGSLibParColor;
}

class WidgetGSLibParColor : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetGSLibParColor(QWidget *parent = 0);
    ~WidgetGSLibParColor();

    void fillFields(GSLibParColor* param);
    void fillFields(GSLibParColor* param, const QString label);
    void updateValue(GSLibParColor* param);

private:
    Ui::WidgetGSLibParColor *ui;
    QColor _color;

private slots:
    void updateColor( int item_index );
};

#endif // WIDGETGSLIBPARCOLOR_H
