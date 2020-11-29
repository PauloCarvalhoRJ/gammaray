#include "v3dcfgwidforattributeinpointset.h"
#include "ui_v3dcfgwidforattributeinpointset.h"
#include "domain/application.h"
#include <vtkActor.h>
#include <vtkProperty.h>

V3DCfgWidForAttributeInPointSet::V3DCfgWidForAttributeInPointSet(PointSet *pointSet,
                                                                 Attribute *attribute,
                                                                 View3DViewData viewObjects,
                                                                 QWidget *parent) :
    View3DConfigWidget(parent),
    ui(new Ui::V3DCfgWidForAttributeInPointSet),
    _viewObjects( viewObjects )
{
    ui->setupUi(this);
}

V3DCfgWidForAttributeInPointSet::~V3DCfgWidForAttributeInPointSet()
{
    delete ui;
}

void V3DCfgWidForAttributeInPointSet::onUserMadeChanges()
{
    vtkSmartPointer<vtkProp> prop = _viewObjects.actor;

    vtkActor* actor = dynamic_cast<vtkActor*>( prop.Get() );

    if( ! actor ){
        Application::instance()->logError("V3DCfgWidForAttributeInPointSet::onUserMadeChanges(): _viewObjects.actor"
                                          " is not a vtkActor.  Cannot set point size to it.");
        return;
    }

    actor->GetProperty()->SetPointSize( ui->spinPointSize->value() );

    emit changed();
}
