#ifndef WIDGETGSLIBPARINT_H
#define WIDGETGSLIBPARINT_H

#include <QWidget>

class GSLibParInt;

namespace Ui {
class WidgetGSLibParInt;
}

class WidgetGSLibParInt : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetGSLibParInt(QWidget *parent = 0);
    ~WidgetGSLibParInt();

    void fillFields(int param);
    void updateValue( uint *param );
    void fillFields( GSLibParInt* param );
    void updateValue( GSLibParInt* param );

    void setLabelText( QString text );

private:
    Ui::WidgetGSLibParInt *ui;
};

#endif // WIDGETGSLIBPARINT_H
