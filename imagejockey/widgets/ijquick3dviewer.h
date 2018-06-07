#ifndef IJQUICK3DVIEWER_H
#define IJQUICK3DVIEWER_H

#include <QWidget>

namespace Ui {
class IJQuick3DViewer;
}

class IJQuick3DViewer;

class IJQuick3DViewer : public QWidget
{
    Q_OBJECT

public:
    /** @param show_not_set If true, adds a "NOT SET" item as the first item. */
    explicit IJQuick3DViewer( QWidget *parent = 0 );
    ~IJQuick3DViewer();

private:
	Ui::IJQuick3DViewer *ui;
};

#endif // IJQUICK3DVIEWER_H
