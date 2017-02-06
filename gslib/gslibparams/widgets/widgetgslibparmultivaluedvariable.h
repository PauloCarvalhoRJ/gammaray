#ifndef WIDGETGSLIBPARMULTIVALUEDVARIABLE_H
#define WIDGETGSLIBPARMULTIVALUEDVARIABLE_H

#include <QWidget>

class GSLibParVarWeight;
class GSLibParMultiValuedVariable;

namespace Ui {
class WidgetGSLibParMultiValuedVariable;
}

class WidgetGSLibParMultiValuedVariable : public QWidget
{
    Q_OBJECT

public:
    /** @param gslib_parameter_type_name Must be one of the names returned by getTypeName() of GSLibParType's subclasses.
        otherwise no widget can be added by the user and an error message appears in program's console.*/
    explicit WidgetGSLibParMultiValuedVariable(const QString gslib_parameter_type_name, QWidget *parent = 0);
    ~WidgetGSLibParMultiValuedVariable();

    void fillFields( QList<GSLibParVarWeight*> *param );
    void updateValue( QList<GSLibParVarWeight*> *param );

    void fillFields( GSLibParMultiValuedVariable *param );
    void updateValue( GSLibParMultiValuedVariable *param );

public slots:
    void onBtnAddClicked(bool);
    void onBtnRemoveClicked(bool);

private:
    Ui::WidgetGSLibParMultiValuedVariable *ui;
    /** This must be one of the names returned by getTypeName() of GSLibParType's subclasses.
        otherwise no widget can be added by the user and an error message appears in program's console.*/
    QString _gslib_parameter_type_name;
    QList<QWidget*> _widgets;
};

#endif // WIDGETGSLIBPARMULTIVALUEDVARIABLE_H
