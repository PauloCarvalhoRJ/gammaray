#include "viewer3dlistwidget.h"

#include <QDragEnterEvent>
#include <QMimeData>

#include "domain/application.h"

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
    Application::instance()->logInfo("Viewer3DListWidget::dropEvent(): Dropped object locator: " + e->mimeData()->text());
}
