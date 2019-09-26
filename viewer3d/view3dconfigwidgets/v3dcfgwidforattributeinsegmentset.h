#ifndef V3DCFGWIDFORATTRIBUTEINSEGMENTSET_H
#define V3DCFGWIDFORATTRIBUTEINSEGMENTSET_H

#include <QWidget>
#include "../view3dconfigwidget.h"
#include "../view3dviewdata.h"

class SegmentSet;
class Attribute;

namespace Ui {
class V3DCfgWidForAttributeInSegmentSet;
}

class V3DCfgWidForAttributeInSegmentSet : public View3DConfigWidget
{
    Q_OBJECT

public:
    explicit V3DCfgWidForAttributeInSegmentSet(
            SegmentSet *segmentSet,
            Attribute *attribute,
            View3DViewData viewObjects,
            QWidget *parent = nullptr);
    ~V3DCfgWidForAttributeInSegmentSet();

private:
    Ui::V3DCfgWidForAttributeInSegmentSet *ui;
    View3DViewData _viewObjects;

private slots:
    void onUserMadeChanges();
};

#endif // V3DCFGWIDFORATTRIBUTEINSEGMENTSET_H
