#include "ijquick3dviewer.h"
#include "ui_ijquick3dviewer.h"

IJQuick3DViewer::IJQuick3DViewer( QWidget *parent ) :
    QWidget(parent),
    ui(new Ui::IJQuick3DViewer)
{
    ui->setupUi(this);
}

IJQuick3DViewer::~IJQuick3DViewer()
{
    delete ui;
}
