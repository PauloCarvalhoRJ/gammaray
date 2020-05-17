#include "thinsectionanalysisdialog.h"
#include "ui_thinsectionanalysisdialog.h"

#include <QFileDialog>
#include <QFileIconProvider>

#include "util.h"

ThinSectionAnalysisDialog::ThinSectionAnalysisDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ThinSectionAnalysisDialog),
    m_directoryPath(""),
    m_qFileIconProvider( new QFileIconProvider() ) //not needed to be a member variable, but construction of QFileIconProvider is heavy
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle( "Thin Section Analysis" );
}

ThinSectionAnalysisDialog::~ThinSectionAnalysisDialog()
{
    delete ui;
    delete m_qFileIconProvider;
}

void ThinSectionAnalysisDialog::onOpenDir()
{
    m_directoryPath = QFileDialog::getExistingDirectory( this, "Directory with images", m_directoryPath );
    onDirectoryChanged();
}

void ThinSectionAnalysisDialog::onDirectoryChanged()
{
    //Update UI to show current directory.
    ui->lblDirectory->setText( m_directoryPath );

    //Build a list of filters for the common image file extensions.
    QStringList exts;
    for( QString ext : Util::getListOfImageFileExtensions() )
        exts << ( "*." + ext );

    //Get a list of image files in current directory (if any).
    QDir dir( m_directoryPath );
    dir.setNameFilters( exts );
    QStringList fileList = dir.entryList();

    //Update the list widget with the image files found in the directory.
    ui->lstFiles->clear();
    for( QString fileName : fileList ){
        QFileInfo fileInfo( m_directoryPath + "/" + fileName );
        QIcon fileIcon = m_qFileIconProvider->icon( fileInfo );
        ui->lstFiles->addItem( new QListWidgetItem( fileIcon, fileName ) );
    }

    //Clear the lists of the images selected as plane and cross polarizations
    ui->lstPlanePolarizationImages->clear();
    ui->lstCrossPolarizationImages->clear();
}

void ThinSectionAnalysisDialog::onPlanePolarizationImageSelected()
{
    ui->lstCrossPolarizationImages->setCurrentRow( ui->lstPlanePolarizationImages->currentRow() );
    onUpdateImageDisplays();
}

void ThinSectionAnalysisDialog::onCrossPolarizationImageSelected()
{
    ui->lstPlanePolarizationImages->setCurrentRow( ui->lstCrossPolarizationImages->currentRow() );
    onUpdateImageDisplays();
}

void ThinSectionAnalysisDialog::onUpdateImageDisplays()
{
    QImage imagePP( ui->lblDirectory->text() + "/" + ui->lstPlanePolarizationImages->currentItem()->text() );
    QPixmap pixPP = QPixmap::fromImage(imagePP);
    ui->lblImgPlanePolarization->setPixmap( pixPP.scaled( ui->lblImgPlanePolarization->size(),
                                                          Qt::KeepAspectRatio,
                                                          Qt::SmoothTransformation) );

    QImage imagePX( ui->lblDirectory->text() + "/" + ui->lstCrossPolarizationImages->currentItem()->text() );
    QPixmap pixPX = QPixmap::fromImage(imagePX);
    ui->lblImgCrossPolarization->setPixmap( pixPX.scaled( ui->lblImgCrossPolarization->size(),
                                                          Qt::KeepAspectRatio,
                                                          Qt::SmoothTransformation) );
}
