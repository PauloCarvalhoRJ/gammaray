#ifndef V3DCFGWIDFORATTRIBUTEINPOINTSET_H
#define V3DCFGWIDFORATTRIBUTEINPOINTSET_H

#include <QWidget>

#include "viewer3d/view3dconfigwidget.h"
#include "viewer3d/view3dviewdata.h"

class PointSet;
class Attribute;

namespace Ui {
class V3DCfgWidForAttributeInPointSet;
}

class V3DCfgWidForAttributeInPointSet : public View3DConfigWidget
{
    Q_OBJECT

public:

    explicit V3DCfgWidForAttributeInPointSet(
            PointSet *pointSet,
            Attribute *attribute,
            View3DViewData viewObjects,
            QWidget *parent = nullptr);

    ~V3DCfgWidForAttributeInPointSet();

private:
    Ui::V3DCfgWidForAttributeInPointSet *ui;
    View3DViewData _viewObjects;

private slots:
    void onUserMadeChanges();
};

#endif // V3DCFGWIDFORATTRIBUTEINPOINTSET_H
