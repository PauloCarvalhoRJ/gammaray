#ifndef WIDGETGSLIBPARMULTIVALUEDFIXED_H
#define WIDGETGSLIBPARMULTIVALUEDFIXED_H

#include <QWidget>

class GSLibParMultiValuedFixed;

namespace Ui {
class WidgetGSLibParMultiValuedFixed;
}

class WidgetGSLibParMultiValuedFixed : public QWidget
{
    Q_OBJECT

public:
    /**
     * @param param_type_names List of GSLib parameter type names.  Thet must be the names returned by getTypeName()
     *  of GSLibParType's subclasses, otherwise no widget can be added by the user and an error message appears
     *  in program's console.
     */
    explicit WidgetGSLibParMultiValuedFixed(QWidget *parent = 0);
    ~WidgetGSLibParMultiValuedFixed();

    void fillFields( GSLibParMultiValuedFixed* param );

    void updateValue( GSLibParMultiValuedFixed* param );

private:
    Ui::WidgetGSLibParMultiValuedFixed *ui;
    QList<QWidget*> _widgets;
};

#endif // WIDGETGSLIBPARMULTIVALUEDFIXED_H
