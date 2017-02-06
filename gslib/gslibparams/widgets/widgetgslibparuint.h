#ifndef WIDGETGSLIBPARUINT_H
#define WIDGETGSLIBPARUINT_H

#include <QWidget>

class GSLibParUInt;

namespace Ui {
class WidgetGSLibParUInt;
}

class WidgetGSLibParUInt : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetGSLibParUInt(QWidget *parent = 0);
    ~WidgetGSLibParUInt();

    void fillFields(GSLibParUInt* param);
    void fillFields(uint value, QString description);
    void updateValue( GSLibParUInt* param );

signals:
    /**
    * This signal notifies client code of value changes by the user.
    * Potential slots recive the new value and the parameter name in the GSLibParFile collection object.
    * If the parameter is anonymous (most of GSLib paramteres are) or the _last_parameter_referenced member
    * is null, the parameter name is an empty string.
    */
    void valueChanged( uint value, QString parameter_name );

private slots:
    void textChanged( const QString &text );

private:
    Ui::WidgetGSLibParUInt *ui;
    /** Saves the GSLibParUInt pointer passed to the last call to the fillFields(GSLibParUInt*) or
     *  updateValue(GSLibParUInt*) methods.  This pointer is null by default. */
    GSLibParUInt* _last_parameter_referenced;
};

#endif // WIDGETGSLIBPARUINT_H
