#ifndef VIEW3DVERTICALEXAGGERATIONWIDGET_H
#define VIEW3DVERTICALEXAGGERATIONWIDGET_H

#include <QWidget>

namespace Ui {
class View3DVerticalExaggerationWidget;
}

class View3DVerticalExaggerationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit View3DVerticalExaggerationWidget(QWidget *parent = 0);
    ~View3DVerticalExaggerationWidget();

    double getVerticalExaggeration();

    virtual void setFocus();

signals:
    void valueChanged(double value);

private:
    Ui::View3DVerticalExaggerationWidget *ui;

private slots:
    void focusChanged( bool in );
};

#endif // VIEW3DVERTICALEXAGGERATIONWIDGET_H
