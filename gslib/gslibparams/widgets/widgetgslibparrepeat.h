#ifndef WIDGETGSLIBPARREPEAT_H
#define WIDGETGSLIBPARREPEAT_H

#include <QWidget>

class GSLibParRepeat;
class GSLibParType;

namespace Ui {
class WidgetGSLibParRepeat;
}

class WidgetGSLibParRepeat : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetGSLibParRepeat(QWidget *parent = 0);
    ~WidgetGSLibParRepeat();

    void fillFields( GSLibParRepeat* param );
    void updateValue( GSLibParRepeat* param );

    /** This is necessary to fully support style sheets in
     * widgets subclassing QWidget directly.
     */
    void paintEvent(QPaintEvent *);

private:
    Ui::WidgetGSLibParRepeat *ui;
    QList<QWidget*> _widgets;
    void add_widgets(QList<GSLibParType*> params_list);
    void update_values(QList<GSLibParType*> params_list, QListIterator<QWidget *>&widget_iterator);
};

#endif // WIDGETGSLIBPARREPEAT_H
