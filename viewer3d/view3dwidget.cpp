//----------Since we're not building with CMake, we need this------------------
#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2) // VTK was built with vtkRenderingOpenGL2
VTK_MODULE_INIT(vtkInteractionStyle)
//-----------------------------------------------------------------------------

#include "view3dwidget.h"
#include "ui_view3dwidget.h"
#include <QVTKWidget.h>

#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
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

    //===========VTK TEST CODE==========================================
    QVTKWidget* vtkwidget = new QVTKWidget();
    vtkSmartPointer<vtkSphereSource> sphereSource =
        vtkSmartPointer<vtkSphereSource>::New();
    vtkSmartPointer<vtkPolyDataMapper> sphereMapper =
        vtkSmartPointer<vtkPolyDataMapper>::New();
    sphereMapper->SetInputConnection( sphereSource->GetOutputPort() );
    vtkSmartPointer<vtkActor> sphereActor =
        vtkSmartPointer<vtkActor>::New();
    sphereActor->SetMapper( sphereMapper );
    vtkSmartPointer<vtkRenderer> renderer =
        vtkSmartPointer<vtkRenderer>::New();
    renderer->AddActor( sphereActor );
    vtkwidget->GetRenderWindow()->AddRenderer( renderer );
    //==================================================================

    ui->frmViewer->layout()->addWidget( vtkwidget );
}

View3DWidget::~View3DWidget()
{
    QSettings qs;
    qs.setValue("viewer3dsplitter", ui->splitter->saveState());
    qs.setValue("viewer3dsplitter2", ui->splitter_2->saveState());
    delete ui;
}
