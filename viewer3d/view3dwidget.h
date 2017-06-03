#ifndef VIEW3DWIDGET_H
#define VIEW3DWIDGET_H

#include <QWidget>

namespace Ui {
class View3DWidget;
}

class View3DWidget : public QWidget
{
    Q_OBJECT

public:
    explicit View3DWidget(QWidget *parent = 0);
    ~View3DWidget();

private:
    Ui::View3DWidget *ui;
};

#endif // VIEW3DWIDGET_H
