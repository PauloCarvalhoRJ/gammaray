#include "viewer3dlistwidget.h"

#include <QDragEnterEvent>
#include <QMenu>
#include <QMimeData>

#include "domain/application.h"
#include "domain/project.h"
#include "domain/projectcomponent.h"

Viewer3DListWidget::Viewer3DListWidget( QWidget *parent ) : QListWidget( parent )
{
    //configure list item context menu
    _contextMenu = new QMenu( this );
    this->setContextMenuPolicy( Qt::CustomContextMenu );
    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onContextMenu(const QPoint &)));
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

    //Create a list item with the object information
    QListWidgetItem* item = new QListWidgetItem( object->getIcon(), object->getNameWithParents() );
    View3DListRecord data( object_locator, getNextInstance( object_locator ) );
    QVariant qv; //the QVariant is necessary to wrap the custom class for storage as list item data
    qv.setValue( data );
    item->setData( Qt::UserRole, qv );
    addItem( item );

    //notify possibly listening contexts of this drop event
    emit newObject( data );
}

uint Viewer3DListWidget::getNextInstance( const QString object_locator )
{
    uint greatest_instance_found = 0;
    for( int i = 0; i < count(); ++i){
        QListWidgetItem* item = this->item( i );
        View3DListRecord data = item->data( Qt::UserRole ).value<View3DListRecord>();
        if( data.objectLocator == object_locator && greatest_instance_found < data.instance )
            greatest_instance_found = data.instance;
    }
    return ++greatest_instance_found;
}

void Viewer3DListWidget::onContextMenu(const QPoint &mouse_location)
{
    QModelIndex index = this->indexAt(mouse_location); //get index to project component under mouse
    //Project* project = Application::instance()->getProject(); //get pointer to open project
    _contextMenu->clear(); //remove all context menu actions

    //get all selected items, this may include other items different from the one under the mouse pointer.
    QModelIndexList selected_indexes = this->selectionModel()->selectedIndexes();

    //if there is just one selected item.
    if( selected_indexes.size() == 1 ){
        //build context menu for any item
        if ( index.isValid() /*&& index.internalPointer() == project->getDataFilesGroup() */) {
            _contextMenu->addAction("Remove from view", this, SLOT(onRemoveFromView()));
        }
    }

    //show the context menu under the mouse cursor.
    if( _contextMenu->actions().size() > 0 )
        _contextMenu->exec(this->mapToGlobal(mouse_location));

}

void Viewer3DListWidget::onRemoveFromView()
{
    QModelIndex index = this->currentIndex();
    QListWidgetItem* item = this->takeItem( index.row() );

    View3DListRecord data = item->data( Qt::UserRole ).value<View3DListRecord>();

    Application::instance()->logInfo( "Viewer3DListWidget::onRemoveFromView(): User requested removal from view of " +
                                      data.objectLocator + "(instance=" + QString::number( data.instance ) + ")" );

    emit removeObject( data );
}
