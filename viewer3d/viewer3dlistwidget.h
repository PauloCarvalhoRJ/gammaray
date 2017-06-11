#ifndef VIEWER3DLISTWIDGET_H
#define VIEWER3DLISTWIDGET_H

#include <QListWidget>

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

    /** Triggered when the user drops a project object. The created style for the object is also passed. */
    void newObject( const QString object_locator, View3DStyle* style );

private:
    /** The styles associated with each item in the list. */
    QList<View3DStyle*> _styles;
};

#endif // VIEWER3DLISTWIDGET_H
