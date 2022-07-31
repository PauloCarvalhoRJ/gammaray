#ifndef V3DCFGWIDFORATTRIBUTEASCONTOURLINES_H
#define V3DCFGWIDFORATTRIBUTEASCONTOURLINES_H

#include <QWidget>
#include "../view3dconfigwidget.h"
#include "../view3dviewdata.h"

class CartesianGrid;
class Attribute;

namespace Ui {
class V3DCfgWidForAttributeAsContourLines;
}

class V3DCfgWidForAttributeAsContourLines : public View3DConfigWidget
{
    Q_OBJECT

public:
    explicit V3DCfgWidForAttributeAsContourLines(
            CartesianGrid *cartesianGrid,
            Attribute *attribute,
            View3DViewData viewObjects,
            QWidget *parent = nullptr);

    ~V3DCfgWidForAttributeAsContourLines();

signals:
    void changed();

private:
    Ui::V3DCfgWidForAttributeAsContourLines *ui;
    View3DViewData _viewObjects;

    /** The color selected by the user to render the contour lines with. */
    QColor m_contourLinesColor;

    /** This must be called to update GUI widgets when this form's data has changed
     * from somewhere else (i.e. the user changed the contour lines color via a dialog. */
    void updateGUI();

private slots:
    void onUserMadeChanges();

    /** Called when the user clicks on the "..." button to open the contours color choosing dialog. */
    void onColorChoose();
};

#endif // V3DCFGWIDFORATTRIBUTEASCONTOURLINES_H
