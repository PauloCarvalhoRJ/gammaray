#include "projecttreeview.h"

#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QApplication>

#include "util.h"
#include "domain/projectcomponent.h"
#include "domain/attribute.h"
#include "domain/file.h"
#include "domain/application.h"

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
    //if LMB is pressed and the user dragged the mouse a certain distance
    // it means that the user wants to start a DND operation
    if (!(e->buttons() & Qt::LeftButton))
        return;
    if ((e->pos() - dragStartPosition).manhattanLength()
            < QApplication::startDragDistance())
        return;

    //get the selected items.
    QModelIndexList selected_indexes = selectionModel()->selectedIndexes();

    //allow DND of just one object at a time
    if( selected_indexes.size() > 1 )
        return;

    /*&& iconLabel->geometry().contains(event->pos())*/

    //access the selected project object
    QModelIndex index = selected_indexes.first();
    ProjectComponent* pc_aspect = static_cast<ProjectComponent*>( index.internalPointer() );

    //create the objects that support the DND operation
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData();

    //set the data (the object locator) to be carried during the DND operation
    mimeData->setText( pc_aspect->getObjectLocator() );
    drag->setMimeData(mimeData);

    //set the icon during DND
    if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI )
        drag->setPixmap( QIcon(":icons32/summary32").pixmap(32, 32) );
    else
        drag->setPixmap( QIcon(":icons/summary16").pixmap(16, 16) );

    //start the DND operation
   /* Qt::DropAction dropAction = */drag->exec();
}

void ProjectTreeView::dragMoveEvent(QDragMoveEvent * /*e*/ ) {
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

void ProjectTreeView::dropEvent(QDropEvent *e)
{
    //user may be dropping data files to add to the project
    foreach (const QUrl &url, e->mimeData()->urls()) {
        QString fileName = url.toLocalFile();
        Application::instance()->addDataFile( fileName );
    }
}
