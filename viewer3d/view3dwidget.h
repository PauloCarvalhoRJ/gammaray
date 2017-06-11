#ifndef VIEW3DWIDGET_H
#define VIEW3DWIDGET_H

#include <QWidget>

#include <vtkSmartPointer.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkActor.h>
#include <QMap>

namespace Ui {
class View3DWidget;
}

class View3DStyle;

class QVTKWidget;

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

    //the VTK renderer (add VTK actors to it to build the scene).
    vtkSmartPointer<vtkRenderer> _renderer;

    //the Qt widget containing a VTK viewport
    QVTKWidget* _vtkwidget;

    //the list of current VTK actors indexed by their associated domain object locators.
    QMap< QString, vtkSmartPointer<vtkActor> > _currentObjects;

private slots:
    void onNewObject( const QString object_locator );
    void onRemoveObject( const QString object_locator );
    void onViewAll();
    void onLookAtXY();
    void onLookAtXZ();
    void onLookAtYZ();
};

#endif // VIEW3DWIDGET_H
