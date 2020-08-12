#ifndef QLISTWIDGETDND_H
#define QLISTWIDGETDND_H

#include <QListWidget>

/**
 * The QListWidgetDnD class extends Qt's QListWidget to enable the drag-and-drop gestures required by GammaRay.
 * This widget expects the pointer to any object of class ProjectComponent (or of its subclasses) to be stored
 * in the QListWidgetItem's data field using the following protocol:
 *   QListWidgetItem* item = new QListWidgetItem( projectObject->getIcon(), projectObject->getName() );
 *   item->setData( Qt::UserRole, reinterpret_cast<uint64_t>( projectObject ) );
 *   ui->listOfVariables->addItem( item );
 * Without a pointer, drag-and-drop will be disabled and the list widget will behave like a normal QListWidget.
 */
class QListWidgetDnD : public QListWidget
{
public:
    QListWidgetDnD( QWidget * parent = nullptr );

    void mousePressEvent( QMouseEvent *e );

    void mouseMoveEvent( QMouseEvent *e);

protected:
    QPoint m_dragStartPosition;
};

#endif // QLISTWIDGETDND_H
