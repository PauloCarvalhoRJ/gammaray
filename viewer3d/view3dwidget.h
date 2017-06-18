#ifndef VIEW3DWIDGET_H
#define VIEW3DWIDGET_H

#include <QWidget>

#include <vtkSmartPointer.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkActor.h>
#include <QMap>

#include "view3dlistrecord.h"
#include "view3dviewdata.h"

namespace Ui {
class View3DWidget;
}

class View3DStyle;

class QVTKWidget;
class QListWidgetItem;
class View3DConfigWidget;

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

    //the list of current VTK actors/visual objects indexed by their associated domain object info.
    QMap< View3DListRecord, View3DViewData > _currentObjects;

    //the list of current 3D viewing config widgets indexed by their associated domain object info.
    QMap< View3DListRecord, View3DConfigWidget* > _currentCfgWidgets;

    //the currently displayed 3D viewing config widget.
    View3DConfigWidget* _currentCfgWidget;

    //removes the current 3D viewing config widget.
    void removeCurrentConfigWidget();

private slots:
    void onNewObject( const View3DListRecord object_info );
    void onRemoveObject( const View3DListRecord object_info );
    void onViewAll();
    void onLookAtXY();
    void onLookAtXZ();
    void onLookAtYZ();
    void onObjectsListItemActivated(QListWidgetItem *item);
    void onConfigWidgetChanged();
};

#endif // VIEW3DWIDGET_H
