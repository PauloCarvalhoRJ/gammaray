#include "pswidget.h"
#include "ui_pswidget.h"
#include "gs/ghostscript.h"
#include "domain/application.h"
#include "domain/project.h"
#include "widgets/qlabelwithcrosshairs.h"
#include "util.h"
#include <QTemporaryFile>
#include <QLabel>
#include <QPixmap>
#include <QBitmap>
#include <QFile>
#include "exceptions/externalprogramexception.h"
#include <QMessageBox>
#include <QScrollBar>
#include <QClipboard>


PSWidget::PSWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PSWidget),
    _current_plot_dpi(80)
{
    ui->setupUi(this);

    _lblImage = new QLabelWithCrossHairs();
    ui->scrollAreaWidgetContents->layout()->addWidget( _lblImage );

    if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI ){
        ui->btn80dpi->setIcon( QIcon(":icons32/80dpi32") );
        ui->btn150dpi->setIcon( QIcon(":icons32/150dpi32") );
        ui->btn300dpi->setIcon( QIcon(":icons32/300dpi32") );
        ui->btn600dpi->setIcon( QIcon(":icons32/600dpi32") );
        ui->btnChangeSettings->setIcon( QIcon(":icons32/setting32") );
        ui->btnCrossHairs->setIcon( QIcon(":icons32/crosshairs32") );
        ui->btnSavePlot->setIcon( QIcon(":icons32/plot32") );
        ui->btnSnapshot->setIcon( QIcon(":icons32/snapshot32") );
        ui->btnZoomIn->setIcon( QIcon(":icons32/zoomin32") );
        ui->btnZoomOut->setIcon( QIcon(":icons32/zoomout32") );
    }
}

PSWidget::~PSWidget()
{
    delete ui;
}

void PSWidget::displayPS(const QString path_to_ps_file)
{
    _path_to_ps_file = path_to_ps_file;

    //generate a temporary file path.
    QTemporaryFile *tmp_file = new QTemporaryFile();
    tmp_file->open();
    QString tmp_file_name = tmp_file->fileName();
    tmp_file->close();
    delete tmp_file; //i'm only interested in the generated file name and path.

    //call Ghostscript to parse PS file and render it as a PNG image file.
    try{
        Ghostscript::makePNG( path_to_ps_file, tmp_file_name, _current_plot_dpi );
        Application::instance()->logInfo( QString("Plot rendered at ").append( QString::number(_current_plot_dpi) ).append(" dpi."));
    } catch ( ExternalProgramException &ex ){
        QMessageBox::critical(this, "GhostScript Error", QString("Failed to run Ghostscript: error code=").append( QString::number(ex.code) ));
    }

    //display the image
    QPixmap pixmap(tmp_file_name);
    _lblImage->setPixmap(pixmap);
    _lblImage->setMask(pixmap.mask());
    _lblImage->setFixedSize(pixmap.size());
    _lblImage->show();

    //scrolls to about the center of plot for convenience
    //TODO: this is not being perfectly centered (work around)
    //      Refer to scroll bar behavior in init time here: http://stackoverflow.com/questions/19836404/qscrollarea-does-not-scroll-to-maximum-after-widgets-have-been-added?rq=1
    ui->scrollImage->verticalScrollBar()->setMinimum(0);
    ui->scrollImage->verticalScrollBar()->setMaximum( pixmap.height() );
    ui->scrollImage->verticalScrollBar()->setValue( (int)(( pixmap.height() - this->height() )/2.0) );
    ui->scrollImage->horizontalScrollBar()->setMinimum(0);
    ui->scrollImage->horizontalScrollBar()->setMaximum( pixmap.width() );
    ui->scrollImage->horizontalScrollBar()->setValue( (int)(( pixmap.width() - this->width() )/2.0) );

    //delete the temporary file
    QFile temporary_png_file( tmp_file_name );
    temporary_png_file.remove();
}

void PSWidget::onIncreasePlotResolution()
{
    _current_plot_dpi += 10;
    this->displayPS( _path_to_ps_file );
}

void PSWidget::onDecreasePlotResolution()
{
    if( _current_plot_dpi > 10 )
        _current_plot_dpi -= 10;
    this->displayPS( _path_to_ps_file );
}

void PSWidget::onTakeSnapshot()
{
    QClipboard *clipboard = QApplication::clipboard();
    const QPixmap px = *(_lblImage->pixmap()); //copy plot pixmap
    //TODO: this gives a flood of error messages when pixmap is not square.
    clipboard->setPixmap( px,  QClipboard::Clipboard );
    QMessageBox::information(this, "Plot snapshot", "The plot image has been copied to the clipboard. You can paste it to any application.");
}

void PSWidget::onSetPlotRes80dpi()
{
    _current_plot_dpi = 80;
    this->displayPS( _path_to_ps_file );
}

void PSWidget::onSetPlotRes150dpi()
{
    _current_plot_dpi = 150;
    this->displayPS( _path_to_ps_file );
}

void PSWidget::onSetPlotRes300dpi()
{
    _current_plot_dpi = 300;
    this->displayPS( _path_to_ps_file );
}

void PSWidget::onSetPlotRes600dpi()
{
    _current_plot_dpi = 600;
    this->displayPS( _path_to_ps_file );
}

void PSWidget::onChangeParameters()
{
    emit userWantsToChangeParameters();
}

void PSWidget::onSavePlot()
{
    emit userWantsToSavePlot();
}

void PSWidget::onShowHideCrossHairs()
{
    _lblImage->toggleCrossHairs();
}
