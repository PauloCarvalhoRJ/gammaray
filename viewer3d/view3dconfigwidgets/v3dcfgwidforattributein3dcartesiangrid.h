#ifndef V3DCFGWIDFORATTRIBUTEIN3DCARTESIANGRID_H
#define V3DCFGWIDFORATTRIBUTEIN3DCARTESIANGRID_H

#include <QWidget>
#include "../view3dconfigwidget.h"

namespace Ui {
class V3DCfgWidForAttributeIn3DCartesianGrid;
}

class V3DCfgWidForAttributeIn3DCartesianGrid : public View3DConfigWidget
{
    Q_OBJECT

public:
    explicit V3DCfgWidForAttributeIn3DCartesianGrid(QWidget *parent = 0);
    ~V3DCfgWidForAttributeIn3DCartesianGrid();

private:
    Ui::V3DCfgWidForAttributeIn3DCartesianGrid *ui;
};

#endif // V3DCFGWIDFORATTRIBUTEIN3DCARTESIANGRID_H
