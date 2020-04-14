#include "verticalproportioncurvedialog.h"
#include "ui_verticalproportioncurvedialog.h"

#include "domain/application.h"
#include <QDragEnterEvent>
#include <QMimeData>

VerticalProportionCurveDialog::VerticalProportionCurveDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VerticalProportionCurveDialog)
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    setAcceptDrops(true);

    setWindowTitle( "Create vertical proportion curves from data." );
}

VerticalProportionCurveDialog::~VerticalProportionCurveDialog()
{
    Application::instance()->logInfo("VerticalProportionCurveDialog destroyed.");
    delete ui;
}

void VerticalProportionCurveDialog::dragEnterEvent(QDragEnterEvent *e)
{
    e->acceptProposedAction();
}

void VerticalProportionCurveDialog::dragMoveEvent(QDragMoveEvent *e)
{
    QPoint eventPos = e->pos();

    //the variables list accepts drops if it comes from somewhere other than lstVariables itself.
    if( ui->lstVariables->geometry().contains( eventPos ) && e->source() != ui->lstVariables )
        e->accept();
    //the trash bin label accepts drops if it comes from the variables list.
    else if( ui->lblTrash->geometry().contains( eventPos ) && e->source() == ui->lstVariables )
        e->accept();
    else
        e->ignore();
}

void VerticalProportionCurveDialog::dropEvent(QDropEvent *e)
{
    //user may be dropping data files to add to the project
    if (e->mimeData()->hasUrls()) {
        foreach (const QUrl &url, e->mimeData()->urls()) {
            QString fileName = url.toLocalFile();
            Application::instance()->addDataFile( fileName );
        }
        return;
    }
}
