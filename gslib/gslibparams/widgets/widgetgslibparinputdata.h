#ifndef WIDGETGSLIBPARINPUTDATA_H
#define WIDGETGSLIBPARINPUTDATA_H

#include <QWidget>

namespace Ui {
class WidgetGSLibParInputData;
}

class GSLibParInputData;
class WidgetGSLibParFile;
class WidgetGSLibParMultiValuedVariable;
class WidgetGSLibParLimitsDouble;

class WidgetGSLibParInputData : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetGSLibParInputData(QWidget *parent = 0);
    ~WidgetGSLibParInputData();

    /** Fills in the widget fields with the values from the given parameter object. */
    void fillFields( GSLibParInputData* param );

    /** Update de values of the given parameter objects with the user-typed values in the widget fields. */
    void updateValue ( GSLibParInputData* param );

    /** This is necessary to fully support style sheets in
     * widgets subclassing QWidget directly.  Otherwise, the CSS code specified
     * in the .ui file (Qt Designer) does not take effect.
     */
    void paintEvent(QPaintEvent *);

private:
    Ui::WidgetGSLibParInputData *ui;
    WidgetGSLibParFile *_wfile;
    WidgetGSLibParMultiValuedVariable *_wmvv;
    WidgetGSLibParLimitsDouble *_wldbl;
};

#endif // WIDGETGSLIBPARINPUTDATA_H
