#ifndef V3DCFGWIDFORATTRIBUTEIN3DCARTESIANGRID_H
#define V3DCFGWIDFORATTRIBUTEIN3DCARTESIANGRID_H

#include <QWidget>
#include "../view3dconfigwidget.h"
#include "../view3dviewdata.h"

class GridFile;
class Attribute;

namespace Ui {
class V3DCfgWidForAttributeIn3DCartesianGrid;
}

class V3DCfgWidForAttributeIn3DCartesianGrid : public View3DConfigWidget
{
    Q_OBJECT

public:
    explicit V3DCfgWidForAttributeIn3DCartesianGrid(
            GridFile *gridFile,
            Attribute *attribute,
            View3DViewData viewObjects,
            QWidget *parent = 0);
    ~V3DCfgWidForAttributeIn3DCartesianGrid();

private:
    Ui::V3DCfgWidForAttributeIn3DCartesianGrid *ui;
    View3DViewData _viewObjects;
    GridFile* m_gridFile;

private slots:
    void onUserMadeChanges();
    void updateLabels();
};

#endif // V3DCFGWIDFORATTRIBUTEIN3DCARTESIANGRID_H
