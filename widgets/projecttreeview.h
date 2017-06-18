#ifndef PROJECTTREEVIEW_H
#define PROJECTTREEVIEW_H

#include <QTreeView>

/** This is a QTreeView subclass tailored to support certain operations on GammaRay project objects.
 * If you use Qt Creator, you can use the "Promote to..." option from the context menu when you right click
 * on the QTreeView on the form's object list. You then have to enter the name of the subclass (ProjectTreeView)
 * and you have to enter the name of new header file (widgets/projecttreeview.h), where the subclass is declared.
 * So you can use this custom class in Qt Creator's Designer.
 */
class ProjectTreeView : public QTreeView
{
public:
    ProjectTreeView( QWidget * parent = nullptr );

    void mousePressEvent( QMouseEvent *e );

    void mouseMoveEvent( QMouseEvent *e);

    void dragMoveEvent(QDragMoveEvent *e);

    void dragEnterEvent(QDragEnterEvent *e);

    void dropEvent( QDropEvent *e );

protected:
    QPoint dragStartPosition;
};

#endif // PROJECTTREEVIEW_H
