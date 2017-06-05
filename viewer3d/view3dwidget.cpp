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

    QVTKWidget* vtkwidget = new QVTKWidget();

    //===========VTK TEST CODE==========================================
    vtkSmartPointer<vtkSphereSource> sphereSource =
        vtkSmartPointer<vtkSphereSource>::New();
    vtkSmartPointer<vtkPolyDataMapper> sphereMapper =
        vtkSmartPointer<vtkPolyDataMapper>::New();
    sphereMapper->SetInputConnection( sphereSource->GetOutputPort() );
    vtkSmartPointer<vtkActor> sphereActor =
        vtkSmartPointer<vtkActor>::New();
    sphereActor->SetMapper( sphereMapper );
    //==================================================================


    vtkSmartPointer<vtkRenderer> renderer =
        vtkSmartPointer<vtkRenderer>::New();

    //add a nice sky-like background
    renderer->GradientBackgroundOn();
    renderer->SetBackground(0.9,0.9,1);
    renderer->SetBackground2(0.5,0.5,1);

    renderer->AddActor( sphereActor );  // VTK TEST CODE
    vtkwidget->GetRenderWindow()->AddRenderer( renderer );

    //----------------------adding the orientation axes-------------------------
    vtkSmartPointer<vtkAxesActor> axes = vtkSmartPointer<vtkAxesActor>::New();
    _vtkAxesWidget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
    _vtkAxesWidget->SetOutlineColor( 0.9300, 0.5700, 0.1300 );
    _vtkAxesWidget->SetOrientationMarker( axes );
    _vtkAxesWidget->SetInteractor( vtkwidget->GetRenderWindow()->GetInteractor() );
    _vtkAxesWidget->SetViewport( 0.0, 0.0, 0.2, 0.2 );
    _vtkAxesWidget->SetEnabled( 1 );
    _vtkAxesWidget->InteractiveOn();
    //--------------------------------------------------------------------------

    //adjusts view so everything fits in the screen
    renderer->ResetCamera();

    //add the VTK widget the layout
    ui->frmViewer->layout()->addWidget( vtkwidget );

    //enable and configure the objects list's drag-and-drop feature.
    //ui->listWidget->setDragEnabled(true);
    //ui->listWidget->setDragDropMode(QAbstractItemView::DragDrop);
    //ui->listWidget->viewport()->setAcceptDrops(true);
    //ui->listWidget->setDropIndicatorShown(true);
    ui->listWidget->setAcceptDrops( true );

}

View3DWidget::~View3DWidget()
{
    QSettings qs;
    qs.setValue("viewer3dsplitter", ui->splitter->saveState());
    qs.setValue("viewer3dsplitter2", ui->splitter_2->saveState());
    delete ui;
}
