//----------Since we're not building with CMake, we need to init the VTK modules------------------
//--------------linking with the VTK libraries is often not enough--------------------------------
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2) // VTK was built with vtkRenderingOpenGL2
VTK_MODULE_INIT(vtkInteractionStyle)
VTK_MODULE_INIT(vtkRenderingFreeType)
//------------------------------------------------------------------------------------------------

#include "view3dwidget.h"
#include "ui_view3dwidget.h"
#include <QVTKWidget.h>

#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkAxesActor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkCamera.h>
#include <QSettings>

#include "domain/application.h"
#include "domain/project.h"
#include "domain/projectcomponent.h"
#include "view3dbuilders.h"
#include "view3dconfigwidget.h"

View3DWidget::View3DWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::View3DWidget),
    _currentCfgWidget( nullptr )
{
    ui->setupUi(this);

    //restore main splitter position
    {
        QSettings qs;
        QByteArray state;
        if( qs.contains( "viewer3dsplitter" ))
            state = qs.value("viewer3dsplitter").toByteArray();
        ui->splitter->restoreState( state );
    }
    //restore left splitter position
    {
        QSettings qs;
        QByteArray state;
        if( qs.contains( "viewer3dsplitter2" ))
            state = qs.value("viewer3dsplitter2").toByteArray();
        ui->splitter_2->restoreState( state );
    }

    _vtkwidget = new QVTKWidget();

    //===========VTK TEST CODE==========================================
//    vtkSmartPointer<vtkSphereSource> sphereSource =
//        vtkSmartPointer<vtkSphereSource>::New();
//    vtkSmartPointer<vtkPolyDataMapper> sphereMapper =
//        vtkSmartPointer<vtkPolyDataMapper>::New();
//    sphereMapper->SetInputConnection( sphereSource->GetOutputPort() );
//    vtkSmartPointer<vtkActor> sphereActor =
//        vtkSmartPointer<vtkActor>::New();
//    sphereActor->SetMapper( sphereMapper );
    //==================================================================


    _renderer = vtkSmartPointer<vtkRenderer>::New();

    //add a nice sky-like background
    _renderer->GradientBackgroundOn();
    _renderer->SetBackground(0.9,0.9,1);
    _renderer->SetBackground2(0.5,0.5,1);

//    renderer->AddActor( sphereActor );  // VTK TEST CODE
    _vtkwidget->GetRenderWindow()->AddRenderer( _renderer );

    //----------------------adding the orientation axes-------------------------
    vtkSmartPointer<vtkAxesActor> axes = vtkSmartPointer<vtkAxesActor>::New();
    _vtkAxesWidget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
    _vtkAxesWidget->SetOutlineColor( 0.9300, 0.5700, 0.1300 );
    _vtkAxesWidget->SetOrientationMarker( axes );
    _vtkAxesWidget->SetInteractor( _vtkwidget->GetRenderWindow()->GetInteractor() );
    _vtkAxesWidget->SetViewport( 0.0, 0.0, 0.2, 0.2 );
    _vtkAxesWidget->SetEnabled( 1 );
    _vtkAxesWidget->InteractiveOn();
    //--------------------------------------------------------------------------

    //adjusts view so everything fits in the screen
    _renderer->ResetCamera();

    //add the VTK widget the layout
    ui->frmViewer->layout()->addWidget( _vtkwidget );

    //enable and configure the objects list's drag-and-drop feature.
    //ui->listWidget->setDragEnabled(true);
    //ui->listWidget->setDragDropMode(QAbstractItemView::DragDrop);
    //ui->listWidget->viewport()->setAcceptDrops(true);
    //ui->listWidget->setDropIndicatorShown(true);
    ui->listWidget->setAcceptDrops( true );

    connect( ui->listWidget, SIGNAL(newObject(View3DListRecord)), this, SLOT(onNewObject(View3DListRecord)) );
    connect( ui->listWidget, SIGNAL(removeObject(View3DListRecord)), this, SLOT(onRemoveObject(View3DListRecord)) );
}

View3DWidget::~View3DWidget()
{
    QSettings qs;
    qs.setValue("viewer3dsplitter", ui->splitter->saveState());
    qs.setValue("viewer3dsplitter2", ui->splitter_2->saveState());
    delete ui;
}

void View3DWidget::removeCurrentConfigWidget()
{
    //if there is a current config widget
    if( _currentCfgWidget ){
        //removes the current config widget
        ui->frmDataViewOptions->layout()->removeWidget( _currentCfgWidget );
        //resets its parent
        _currentCfgWidget->setParent( nullptr );
        //resets the pointer
        _currentCfgWidget = nullptr;
    }
}

void View3DWidget::onNewObject( const View3DListRecord object_info )
{
    Application::instance()->logInfo("View3DWidget::onNewObject(): new object to display: " + object_info.getDescription());

    View3DViewData viewData = Application::instance()->
            getProject()->
            findObject( object_info.objectLocator )->
            build3DViewObjects( this );

    //gets the VTK Actor that represents the domain object
    vtkSmartPointer<vtkProp> actor = viewData.actor;

    //adds the actor for viewing
    _renderer->AddActor( actor );

    //redraw the scene
    _vtkwidget->update();

    //keeps a list of locator-actor pairs to allow management
    _currentObjects.insert( object_info, viewData );
}

void View3DWidget::onRemoveObject( const View3DListRecord object_info )
{
    //removes the VTK actor matching the object locator from the list.
    vtkSmartPointer<vtkProp> actor = _currentObjects.take( object_info ).actor;

    //removes the VTK actor from view.
    _renderer->RemoveActor( actor );

    //redraw the scene
    _vtkwidget->update();

    removeCurrentConfigWidget();

    //removes and deletes the config widget (if any) associated with the object
    View3DConfigWidget * widget = nullptr;
    if( _currentCfgWidgets.contains( object_info )){
        widget = _currentCfgWidgets.take( object_info );
        delete widget;
    }

    //TODO: verify whether the smart pointer manages memory after all local references to the actor have been removed.
}

void View3DWidget::onViewAll()
{
    //adjusts view so everything fits in the screen
    _renderer->ResetCamera();
    //redraw the scene
    _vtkwidget->update();
}

void View3DWidget::onLookAtXY()
{
    //_renderer->ResetCamera();
    double *fp = _renderer->GetActiveCamera()->GetFocalPoint();
    double *p = _renderer->GetActiveCamera()->GetPosition();
    double dist = std::sqrt( (p[0]-fp[0])*(p[0]-fp[0]) + (p[1]-fp[1])*(p[1]-fp[1]) + (p[2]-fp[2])*(p[2]-fp[2]) );
    _renderer->GetActiveCamera()->SetPosition(fp[0], fp[1], fp[2]+dist);
    _renderer->GetActiveCamera()->SetViewUp(0.0, 1.0, 0.0);
    //redraw the scene
    _vtkwidget->update();
}

void View3DWidget::onLookAtXZ()
{
    double *fp = _renderer->GetActiveCamera()->GetFocalPoint();
    double *p = _renderer->GetActiveCamera()->GetPosition();
    double dist = std::sqrt( (p[0]-fp[0])*(p[0]-fp[0]) + (p[1]-fp[1])*(p[1]-fp[1]) + (p[2]-fp[2])*(p[2]-fp[2]) );
    _renderer->GetActiveCamera()->SetPosition(fp[0], fp[1]-dist, fp[2]);
    _renderer->GetActiveCamera()->SetViewUp(0.0, 0.0, 1.0);
    //redraw the scene
    _vtkwidget->update();
}

void View3DWidget::onLookAtYZ()
{
    double *fp = _renderer->GetActiveCamera()->GetFocalPoint();
    double *p = _renderer->GetActiveCamera()->GetPosition();
    double dist = std::sqrt( (p[0]-fp[0])*(p[0]-fp[0]) + (p[1]-fp[1])*(p[1]-fp[1]) + (p[2]-fp[2])*(p[2]-fp[2]) );
    _renderer->GetActiveCamera()->SetPosition(fp[0]+dist, fp[1], fp[2]);
    _renderer->GetActiveCamera()->SetViewUp(0.0, 0.0, 1.0);
    //redraw the scene
    _vtkwidget->update();
}

void View3DWidget::onObjectsListItemActivated(QListWidgetItem *item)
{
    //retrieve the selected item's object info
    View3DListRecord object_info = item->data( Qt::UserRole ).value<View3DListRecord>();

    //fetch the object's pointer
    ProjectComponent* object = Application::instance()->getProject()->findObject( object_info.objectLocator );

    //removes the current view config widget
    removeCurrentConfigWidget();

    //if an object was found
    if( object ){
        Application::instance()->logInfo("View3DWidget::onObjectsListItemActivated(): object found: " +
                                          object->getName() + ".");
        //retrieve or create the config widget for the object
        View3DConfigWidget *widget = nullptr;
        if( _currentCfgWidgets.contains( object_info ) ){
            widget = _currentCfgWidgets[object_info];
        } else {
            View3DViewData viewObjects = _currentObjects[object_info];
            widget = object->build3DViewerConfigWidget( viewObjects );
            if( widget ){
                _currentCfgWidgets.insert( object_info, widget );
                //connects the signal/slot upon user changes
                connect( widget, SIGNAL(changed()), this, SLOT(onConfigWidgetChanged()));
            }
        }
        //if there is a config widget
        if( widget ){
            //places the config widget in the interface
            ui->frmDataViewOptions->layout()->addWidget( widget );
            //sets as the current config widget
            _currentCfgWidget = widget;
        } else {
            Application::instance()->logError("View3DWidget::onObjectsListItemActivated(): null widget returned.");
        }
    } else {
        Application::instance()->logError("View3DWidget::onObjectsListItemActivated(): object with locator/instance " +
                                          object_info.getDescription() + " not found.");
    }
}

void View3DWidget::onConfigWidgetChanged()
{
    Application::instance()->logInfo("View3DWidget::onConfigWidgetChanged()");
    _renderer->Render();
    _vtkwidget->update();
}
