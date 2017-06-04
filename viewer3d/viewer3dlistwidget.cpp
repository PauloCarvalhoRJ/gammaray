#include "viewer3dlistwidget.h"

#include <QDragEnterEvent>

#include "domain/application.h"

Viewer3DListWidget::Viewer3DListWidget( QWidget *parent ) : QListWidget( parent )
{
}

void Viewer3DListWidget::dragMoveEvent(QDragMoveEvent *e) {
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
    //read mimeData() from QDropEvent
    Application::instance()->logInfo("DROP EVENT");
}
