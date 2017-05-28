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

View3DWidget::View3DWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::View3DWidget)
{
    ui->setupUi(this);

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

    layout()->addWidget( vtkwidget );
}

View3DWidget::~View3DWidget()
{
    delete ui;
}
