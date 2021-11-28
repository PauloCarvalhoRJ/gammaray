#ifndef VIEW3DWIDGET_H
#define VIEW3DWIDGET_H

#include <QWidget>

#include <QMap>
#include <vtkSmartPointer.h>
#include <vtkObject.h>

class vtkActor;
class vtkOrientationMarkerWidget;

#include "view3dlistrecord.h"
#include "view3dviewdata.h"

namespace Ui
{
class View3DWidget;
}

class QVTKOpenGLWidget;
class QListWidgetItem;
class View3DConfigWidget;
class View3DVerticalExaggerationWidget;
class vtkRenderer;
class vtkDistanceWidget;
class View3DTextConfigWidget;
class v3dMouseInteractor;

class View3DWidget : public QWidget
{
    Q_OBJECT

public:
    explicit View3DWidget(QWidget *parent = 0);
    ~View3DWidget();

    /** Returns the VTK renderer used to paint the scene on this widget's canvas. */
    vtkSmartPointer<vtkRenderer> getRenderer() { return _rendererMainScene; }

    /** Returns the current vertical exaggeration setting, */
    double getVerticalExaggeration() const;

private:
    Ui::View3DWidget *ui;

    // this must be class variable, otherwise a crash ensues due to smart pointer going
    // out of scope
    vtkSmartPointer<vtkOrientationMarkerWidget> _vtkAxesWidget;

    // the main VTK renderer (add VTK actors of geo-objects to it to build the scene).
    vtkSmartPointer<vtkRenderer> _rendererMainScene;

    // the VTK renderer for always-on-top objects (e.g. labels, symbols, etc.) so they
    // are not occluded by objects in the main scene.
    vtkSmartPointer<vtkRenderer> _rendererForeground;

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

    // the floating widget for configuring the text labels in the 3D scene.
    View3DTextConfigWidget *_textConfigWiget;

    // The class that customizes mouse events to provide a custo mouse interacation.
    vtkSmartPointer<v3dMouseInteractor> m_myInteractor;

    // The distance measuring tool.
    vtkSmartPointer<vtkDistanceWidget> m_distanceWidget;

    // If this is false, parallel projection is used.
    bool m_perspectiveProjection;

    // removes the current 3D viewing config widget.
    void removeCurrentConfigWidget();

    // this is called whenever the an event occurs in the vtkRenderer.
    static void rendererCallback( vtkObject* caller,
                                  unsigned long vtkNotUsed(event),
                                  void* arg,
                                  void* vtkNotUsed(whatIsThis) );

    // applies the current text style set by the user to the passed text actor.
    void applyCurrentTextStyle( vtkSmartPointer<vtkBillboardTextActor3D> textActor );

private Q_SLOTS:
    void onNewObject(const View3DListRecord object_info);
    void onRemoveObject(const View3DListRecord object_info);
    void onShowHideObject(const View3DListRecord object_info, bool show);
    void onViewAll();
    void onLookAtXY();
    void onLookAtXZ();
    void onLookAtYZ();
    void onObjectsListItemActivated(QListWidgetItem *item);
    void onConfigWidgetChanged();
    void onVerticalExaggeration();
    void onVerticalExaggerationChanged(double value);
    void onTextStyle();
    void onTextConfigChanged();
    void onRuler();
    void onProjection();
};

#endif // VIEW3DWIDGET_H
