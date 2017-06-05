#ifndef VIEW3DWIDGET_H
#define VIEW3DWIDGET_H

#include <QWidget>

#include <vtkSmartPointer.h>
#include <vtkOrientationMarkerWidget.h>

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

    //this must be class variable, otherwise a crash ensues due to smart pointer going out of scope
    vtkSmartPointer<vtkOrientationMarkerWidget> _vtkAxesWidget;
};

#endif // VIEW3DWIDGET_H
