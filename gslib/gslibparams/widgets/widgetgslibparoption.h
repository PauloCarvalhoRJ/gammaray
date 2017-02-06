#ifndef WIDGETGSLIBPAROPTION_H
#define WIDGETGSLIBPAROPTION_H

#include <QWidget>

class GSLibParOption;

namespace Ui {
class WidgetGSLibParOption;
}

class WidgetGSLibParOption : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetGSLibParOption(QWidget *parent = 0);
    ~WidgetGSLibParOption();
    void fillFields(GSLibParOption *param);
    void fillFields(GSLibParOption *param, QString description);
    void updateValue(GSLibParOption *param);
private:
    Ui::WidgetGSLibParOption *ui;
};

#endif // WIDGETGSLIBPAROPTION_H
