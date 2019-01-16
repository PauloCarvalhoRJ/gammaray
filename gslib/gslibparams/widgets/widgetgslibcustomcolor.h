#ifndef WIDGETGSLIBCUSTOMCOLOR_H
#define WIDGETGSLIBCUSTOMCOLOR_H

#include <QWidget>

class GSLibParCustomColor;

namespace Ui {
class WidgetGSLibCustomColor;
}

class WidgetGSLibCustomColor : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetGSLibCustomColor(QWidget *parent = nullptr);
    ~WidgetGSLibCustomColor();

    void fillFields(GSLibParCustomColor* param);
    void updateValue(GSLibParCustomColor* param);

private:
    Ui::WidgetGSLibCustomColor *ui;
    QColor _color;

private Q_SLOTS:
    void updateColorSample( int value );
    void openColorChooser();
};

#endif // WIDGETGSLIBCUSTOMCOLOR_H
