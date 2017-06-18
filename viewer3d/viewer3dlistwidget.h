#ifndef VIEWER3DLISTWIDGET_H
#define VIEWER3DLISTWIDGET_H

#include <QListWidget>
#include "view3dlistrecord.h"

class View3DStyle;

/** This is a QListWidget subclass tailored to support certain operations on GammaRay project objects.
 * If you use Qt Creator, you can use the "Promote to..." option from the context menu when you right click
 * on the QListWidget on the form's object list. You then have to enter the name of the subclass (Viewer3DListWidget)
 * and you have to enter the name of new header file (viewer3dlistwidget.h), where the subclass is declared.
 * So you can use this custom class in QtCreator's Designer.
 */
class Viewer3DListWidget : public QListWidget
{

    Q_OBJECT

public:

    Viewer3DListWidget(QWidget *parent = nullptr);

    void dragMoveEvent(QDragMoveEvent *e);

    void dragEnterEvent(QDragEnterEvent *e);

    void dropEvent(QDropEvent *e);

signals:

    /** Triggered when the user drops a project object. */
    void newObject( const View3DListRecord object_info );

    /** Triggered when the user removes an object from view. */
    void removeObject( const View3DListRecord object_info );

private:
    //context menu for the list items
    QMenu *_contextMenu;

    /** Returns an increasing number of repeated occurences of the same object_locator.
     * The user may display the same object multiple times, possibly in different ways (e.g.
     * view the same Attribute in a 3D Cartesian grid as a translucent or clipped data cube and as iso-surface).
     */
    uint getNextInstance( const QString object_locator );

private slots:
    void onContextMenu(const QPoint &mouse_location);
    void onRemoveFromView();
};

#endif // VIEWER3DLISTWIDGET_H
