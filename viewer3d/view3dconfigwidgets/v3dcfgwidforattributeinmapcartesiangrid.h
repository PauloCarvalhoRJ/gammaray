#ifndef V3DCFGWIDFORATTRIBUTEINMAPCARTESIANGRID_H
#define V3DCFGWIDFORATTRIBUTEINMAPCARTESIANGRID_H

#include <QWidget>
#include "../view3dconfigwidget.h"
#include "../view3dviewdata.h"

class CartesianGrid;
class Attribute;

namespace Ui {
class V3DCfgWidForAttributeInMapCartesianGrid;
}

class V3DCfgWidForAttributeInMapCartesianGrid : public View3DConfigWidget
{
    Q_OBJECT

public:
    explicit V3DCfgWidForAttributeInMapCartesianGrid(
            CartesianGrid *cartesianGrid,
            Attribute *attribute,
            View3DViewData viewObjects,
            QWidget *parent = 0);

    ~V3DCfgWidForAttributeInMapCartesianGrid();

signals:
    void changed();

private:
    Ui::V3DCfgWidForAttributeInMapCartesianGrid *ui;
    View3DViewData _viewObjects;

private slots:
    void onUserMadeChanges();
};

#endif // V3DCFGWIDFORATTRIBUTEINMAPCARTESIANGRID_H
