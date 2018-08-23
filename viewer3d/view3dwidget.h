#ifndef VIEW3DWIDGET_H
#define VIEW3DWIDGET_H

#include <QWidget>

#include <QMap>
#include <vtkSmartPointer.h>

class vtkActor;
class vtkOrientationMarkerWidget;

#include "view3dlistrecord.h"
#include "view3dviewdata.h"

namespace Ui
{
class View3DWidget;
}

class View3DStyle;

class QVTKOpenGLWidget;
class QListWidgetItem;
class View3DConfigWidget;
class View3DVerticalExaggerationWidget;
class vtkRenderer;

class View3DWidget : public QWidget
{
    Q_OBJECT

public:
    explicit View3DWidget(QWidget *parent = 0);
    ~View3DWidget();

    /** Returns the VTK renderer used to paint the scene on this widget's canvas. */
    vtkSmartPointer<vtkRenderer> getRenderer() { return _renderer; }

private:
    Ui::View3DWidget *ui;

    // this must be class variable, otherwise a crash ensues due to smart pointer going
    // out of scope
    vtkSmartPointer<vtkOrientationMarkerWidget> _vtkAxesWidget;

    // the VTK renderer (add VTK actors to it to build the scene).
    vtkSmartPointer<vtkRenderer> _renderer;

    // the Qt widget containing a VTK viewport
    QVTKOpenGLWidget *_vtkwidget;

    // the list of current VTK actors/visual objects indexed by their associated domain
    // object info.
    QMap<View3DListRecord, View3DViewData> _currentObjects;

    // the list of current 3D viewing config widgets indexed by their associated domain
    // object info.
    QMap<View3DListRecord, View3DConfigWidget *> _currentCfgWidgets;

    // the currently displayed 3D viewing config widget.
    View3DConfigWidget *_currentCfgWidget;

    // the floating widget for configuring the vertical scale.
    View3DVerticalExaggerationWidget *_verticalExaggWiget;

    // removes the current 3D viewing config widget.
    void removeCurrentConfigWidget();

private slots:
    void onNewObject(const View3DListRecord object_info);
    void onRemoveObject(const View3DListRecord object_info);
    void onViewAll();
    void onLookAtXY();
    void onLookAtXZ();
    void onLookAtYZ();
    void onObjectsListItemActivated(QListWidgetItem *item);
    void onConfigWidgetChanged();
    void onVerticalExaggeration();
    void onVerticalExaggerationChanged(double value);
};

#endif // VIEW3DWIDGET_H
