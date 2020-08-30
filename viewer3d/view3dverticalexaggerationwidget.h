#ifndef VIEW3DVERTICALEXAGGERATIONWIDGET_H
#define VIEW3DVERTICALEXAGGERATIONWIDGET_H

#include <QWidget>

namespace Ui {
class View3DVerticalExaggerationWidget;
}

/** This is a floating widget to allow the user to quickly set vertical exaggeration without having to open
 * and close a dialog, minimizing mouse clicks.
 */
class View3DVerticalExaggerationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit View3DVerticalExaggerationWidget(QWidget *parent = 0);
    ~View3DVerticalExaggerationWidget();

    /** Returns the vertical exaggeration set by the user. */
    double getVerticalExaggeration();

    /** Method involved in floating widget dynamics.  This is normally not called
     * by client code.
     */
    virtual void setFocus();

signals:
    void valueChanged(double value);

private:
    Ui::View3DVerticalExaggerationWidget *ui;

private slots:
    /** Method involved in floating widget dynamics.  This is normally not called
     * by client code.
     */
    void focusChanged( bool in );
};

#endif // VIEW3DVERTICALEXAGGERATIONWIDGET_H
