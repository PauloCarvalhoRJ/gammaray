#ifndef WIDGETGSLIBPARVMODEL_H
#define WIDGETGSLIBPARVMODEL_H

#include <QWidget>

namespace Ui {
class WidgetGSLibParVModel;
}

class GSLibParVModel;

class WidgetGSLibParVModel : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetGSLibParVModel(QWidget *parent = 0);
    ~WidgetGSLibParVModel();

    void fillFields( GSLibParVModel* param );
    void updateValue( GSLibParVModel* param );

private:
    Ui::WidgetGSLibParVModel *ui;
    QList<QWidget*> _widgets;
};

#endif // WIDGETGSLIBPARVMODEL_H
