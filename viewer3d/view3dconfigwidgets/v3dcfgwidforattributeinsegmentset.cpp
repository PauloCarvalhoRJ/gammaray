#include "v3dcfgwidforattributeinsegmentset.h"
#include "ui_v3dcfgwidforattributeinsegmentset.h"
#include <vtkTubeFilter.h>

V3DCfgWidForAttributeInSegmentSet::V3DCfgWidForAttributeInSegmentSet(SegmentSet *segmentSet,
                                                                     Attribute *attribute,
                                                                     View3DViewData viewObjects,
                                                                     QWidget *parent) :
    View3DConfigWidget(parent),
    ui(new Ui::V3DCfgWidForAttributeInSegmentSet),
    _viewObjects( viewObjects )
{
    ui->setupUi(this);
}

V3DCfgWidForAttributeInSegmentSet::~V3DCfgWidForAttributeInSegmentSet()
{
    delete ui;
}

void V3DCfgWidForAttributeInSegmentSet::onUserMadeChanges()
{
    vtkSmartPointer<vtkTubeFilter> tubeFilter = _viewObjects.tubeFilter;

    bool ok;
    double radius = ui->txtDiameter->text().toDouble( &ok );
    if( ! ok )
        return;

    tubeFilter->SetRadius( radius );
    tubeFilter->Update();

    emit changed();
}
