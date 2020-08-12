#include "qlistwidgetdnd.h"

#include "util.h"
#include "domain/projectcomponent.h"
#include "domain/application.h"

#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>

QListWidgetDnD::QListWidgetDnD( QWidget *parent ) : QListWidget( parent )
{

}

void QListWidgetDnD::mousePressEvent(QMouseEvent *e)
{
    //capture the initial drag position
    if ( e->button() == Qt::LeftButton )
        m_dragStartPosition = e->pos();

    //forward the event to the superclass.
    QListWidget::mousePressEvent( e );
}

void QListWidgetDnD::mouseMoveEvent(QMouseEvent *e)
{
    //if LMB is pressed and the user dragged the mouse a certain distance
    // it means that the user wants to start a DND operation
    if (!(e->buttons() & Qt::LeftButton))
        return;
    if ((e->pos() - m_dragStartPosition).manhattanLength()
            < QApplication::startDragDistance())
        return;

    //get the selected items.
    const QList< QListWidgetItem* >& selected_items = selectedItems();

    //allow DND of just one object at a time
    if( selected_items.size() != 1 )
        return;

    //access the selected project object
    QListWidgetItem* selected_item = selected_items.first();

    bool ok;
    uint64_t crudePointer = selected_item->data(Qt::UserRole).toULongLong( &ok );
    if( !ok )
        return;

    ProjectComponent* pc_aspect = reinterpret_cast<ProjectComponent*>( crudePointer );

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
