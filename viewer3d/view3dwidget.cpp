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
#include <QSettings>

#include "domain/application.h"
#include "domain/project.h"
#include "domain/projectcomponent.h"
#include "view3dbuilders.h"

View3DWidget::View3DWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::View3DWidget)
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

    connect( ui->listWidget, SIGNAL(newObject(QString)), this, SLOT(onNewObject(QString)) );
    connect( ui->listWidget, SIGNAL(removeObject(QString)), this, SLOT(onRemoveObject(QString)) );
}

View3DWidget::~View3DWidget()
{
    QSettings qs;
    qs.setValue("viewer3dsplitter", ui->splitter->saveState());
    qs.setValue("viewer3dsplitter2", ui->splitter_2->saveState());
    delete ui;
}

void View3DWidget::onNewObject(const QString object_locator)
{
    Application::instance()->logInfo("View3DWidget::onNewObject(): new object to display: " + object_locator);

    //gets the VTK Actor that represents the domain object
    vtkSmartPointer<vtkActor> actor = Application::instance()->getProject()->findObject( object_locator )->buildVTKActor();

    //adds the actor for viewing
    _renderer->AddActor( actor );

    //adjusts view so everything fits in the screen
    _renderer->ResetCamera();

    //redraw the scene
    _vtkwidget->update();

    //keeps a list of locator-actor pairs to allow management
    _currentObjects.insert( object_locator, actor );
}

void View3DWidget::onRemoveObject(const QString object_locator)
{
    //removes the VTK actor matching the object locator from the list.
    vtkSmartPointer<vtkActor> actor = _currentObjects.take( object_locator );

    //removes the VTK actor from view.
    _renderer->RemoveActor( actor );

    //adjusts view so everything fits in the screen
    _renderer->ResetCamera();

    //redraw the scene
    _vtkwidget->update();

    //TODO: verify whether the smart pointer manages memory after all local references to the actor have been removed.
}

void View3DWidget::onViewAll()
{
    //adjusts view so everything fits in the screen
    _renderer->ResetCamera();
    //redraw the scene
    _vtkwidget->update();
}
