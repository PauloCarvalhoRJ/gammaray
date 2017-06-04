#include "projecttreeview.h"

#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QApplication>

#include "util.h"

ProjectTreeView::ProjectTreeView(QWidget *parent) : QTreeView( parent )
{
}

void ProjectTreeView::mousePressEvent(QMouseEvent *e)
{
    //capture the initial drap position
    if (e->button() == Qt::LeftButton)
        dragStartPosition = e->pos();

    //let whatever it is supposed to happen with QTreeView.
    QTreeView::mousePressEvent( e );
}

void ProjectTreeView::mouseMoveEvent(QMouseEvent *e)
{
    if (!(e->buttons() & Qt::LeftButton))
        return;
    if ((e->pos() - dragStartPosition).manhattanLength()
            < QApplication::startDragDistance())
        return;

    /*&& iconLabel->geometry().contains(event->pos())*/

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData();

    mimeData->setText("commentEdit->toPlainText()");
    drag->setMimeData(mimeData);
    if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI )
        drag->setPixmap( QIcon(":icons32/summary32").pixmap(32, 32) );
    else
        drag->setPixmap( QIcon(":icons/summary16").pixmap(16, 16) );

    Qt::DropAction dropAction = drag->exec();
}

void ProjectTreeView::dragMoveEvent(QDragMoveEvent *e) {
    /*e->acceptProposedAction();
    if (e->source() != this) {
        e->accept();
    } else {
        e->accept();
    }*/
}

void ProjectTreeView::dragEnterEvent(QDragEnterEvent *e)
{
    e->acceptProposedAction();
}
