#include "viewer3dlistwidget.h"

#include <QDragEnterEvent>
#include <QMimeData>

#include "domain/application.h"
#include "domain/project.h"
#include "domain/projectcomponent.h"
#include "view3dstyle.h"

Viewer3DListWidget::Viewer3DListWidget( QWidget *parent ) : QListWidget( parent )
{
}

void Viewer3DListWidget::dragMoveEvent(QDragMoveEvent * /*e*/) {
    //e->acceptProposedAction();
//    if (e->source() != this) {
//        e->accept();
//    } else {
//        e->accept();
//    }
}

void Viewer3DListWidget::dragEnterEvent(QDragEnterEvent *e)
{
    e->acceptProposedAction();
}

void Viewer3DListWidget::dropEvent(QDropEvent *e)
{
    //user may be dropping data files to add to the project
    if (e->mimeData()->hasUrls()) {
        foreach (const QUrl &url, e->mimeData()->urls()) {
            QString fileName = url.toLocalFile();
            Application::instance()->addDataFile( fileName );
        }
        return;
    }

    //otherwise, they are project objects
    //inform user that an object was dropped
    QString object_locator = e->mimeData()->text();
    Application::instance()->logInfo("Viewer3DListWidget::dropEvent(): Dropped object locator: " + object_locator);
    ProjectComponent* object = Application::instance()->getProject()->findObject( object_locator );
    if( ! object ){
        Application::instance()->logError("Viewer3DListWidget::dropEvent(): object not found. Drop operation failed.");
        return;
    } else {
        Application::instance()->logInfo("Viewer3DListWidget::dropEvent(): Found object: " + object->getName());
    }

    //Create a list item and a visual style corresponding to the object. (the same object may be represented multiple times)
    QListWidgetItem* item = new QListWidgetItem( object->getIcon(), object->getName() );
    View3DStyle* style = new View3DStyle();
    item->setData( Qt::UserRole, object_locator );
    addItem( item );
    _styles.append( style );

    emit newObject( object_locator, style );
}
