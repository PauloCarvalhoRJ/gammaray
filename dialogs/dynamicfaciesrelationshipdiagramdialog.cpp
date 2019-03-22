#include "dynamicfaciesrelationshipdiagramdialog.h"
#include "ui_dynamicfaciesrelationshipdiagramdialog.h"

#include "gs/ghostscript.h"
#include "exceptions/externalprogramexception.h"
#include "domain/application.h"
#include "domain/project.h"

#include <QMessageBox>
#include <QProgressDialog>
#include <QBitmap>
#include <QScrollBar>
#include <QClipboard>
#include <QInputDialog>

DynamicFaciesRelationshipDiagramDialog::DynamicFaciesRelationshipDiagramDialog(
        std::vector<Attribute *> &categoricalAttributes,
        double hInitial,
        double hFinal,
        int nSteps,
        double toleranceCoefficient,
        QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DynamicFaciesRelationshipDiagramDialog),
    m_categoricalAttributes( categoricalAttributes ),
    m_hInitial( hInitial ),
    m_hFinal( hFinal ),
    m_nSteps( nSteps ),
    m_toleranceCoefficient( toleranceCoefficient )
{
    ui->setupUi(this);

    setWindowTitle("Dynamic Facies Relationship Diagram Dialog");

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    //updates the labels
    ui->lblMin->setText( " h = " + QString::number( m_hInitial ) );
    ui->lblMax->setText( QString::number( m_hFinal ) );

    onGenerateDiagrams();
}

DynamicFaciesRelationshipDiagramDialog::~DynamicFaciesRelationshipDiagramDialog()
{
    delete ui;
}

void DynamicFaciesRelationshipDiagramDialog::onDisplayDiagram(int index)
{
    if( index >= m_images.size() ){
        Application::instance()->logWarn("DynamicFaciesRelationshipDiagramDialog::onDisplayDiagram(): invalid diagram index: " + QString::number( index ) );
        return;
    }

    QPixmap& pixmap = m_images[index];

    ui->lblCurrentH->setText( "h = " + QString::number( m_hFTMs[index].first ) );

    ui->lblImage->setPixmap(pixmap);
    ui->lblImage->setMask(pixmap.mask());
    ui->lblImage->setFixedSize(pixmap.size());
    ui->lblImage->show();

    //scrolls to the bottom of plot for convenience
    ui->scrollAreaImage->verticalScrollBar()->setMinimum(0);
    ui->scrollAreaImage->verticalScrollBar()->setMaximum( pixmap.height() );
    ui->scrollAreaImage->verticalScrollBar()->setValue( pixmap.height() );
    ui->scrollAreaImage->horizontalScrollBar()->setMinimum(0);
    ui->scrollAreaImage->horizontalScrollBar()->setMaximum( pixmap.width() );
    ui->scrollAreaImage->horizontalScrollBar()->setValue( (int)(( pixmap.width() - this->width() )/2.0) );

}

void DynamicFaciesRelationshipDiagramDialog::onGenerateDiagrams()
{
    //clear the image cache
    m_images.clear();

    //compute the FTMs for all separations
    m_hFTMs = Util::computeFaciesTransitionMatrices( m_categoricalAttributes,
                                                     m_hInitial,
                                                     m_hFinal,
                                                     m_nSteps,
                                                     m_toleranceCoefficient );

    //compress the FTMs
    Util::compressFaciesTransitionMatrices( m_hFTMs );

    //show a progress dialog
    QProgressDialog progressDialog;
    progressDialog.show();
    progressDialog.setLabelText("Building facies relationship diagrams...");
    progressDialog.setMinimum(0);
    progressDialog.setValue(0);
    progressDialog.setMaximum( m_hFTMs.size()-1 );

    //for each FTM
    int iProgress = 0;
    for( hFTM& hftm : m_hFTMs ){

        progressDialog.setValue( iProgress );
        QApplication::processEvents(); //let Qt do repainting

        //plot a Facies Relationship Diagram for a FTM as a Postscript file
        QString psFilePath;
        Util::makeFaciesRelationShipDiagramPlot( hftm.second,
                                                 psFilePath,
                                                 ui->dblSpinCutoff->value(),
                                                 ui->chkMakeProportional->isChecked(),
                                                 ui->spinPrecision->value(),
                                                 ui->spinMaxLineThickness->value() );

        try{

            //render the Postscript file as a PNG image.
            QString png_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("png");
            Ghostscript::makePNG( psFilePath, png_file_path, ui->spinDPI->value() );

            //load and cache the image
            m_images.push_back( QPixmap( png_file_path ) );

            //delete the temporary files
            if( ! QFile::remove( png_file_path ) )
                Application::instance()->logWarn("DynamicFaciesRelationshipDiagramDialog::DynamicFaciesRelationshipDiagramDialog(): Failed to delete temporary file " + png_file_path );
            if( ! QFile::remove( psFilePath ) )
                Application::instance()->logWarn("DynamicFaciesRelationshipDiagramDialog::DynamicFaciesRelationshipDiagramDialog(): Failed to delete temporary file " + psFilePath );

        } catch ( ExternalProgramException &ex ){
            QMessageBox::critical(this, "GhostScript Error", QString("Failed to run Ghostscript: error code=").append( QString::number(ex.code) ));
        }

        ++iProgress;
    }

    //readying the slider control
    ui->sliderH->setMinimum( 0 );
    ui->sliderH->setMaximum( m_images.size()-1 );
    ui->sliderH->setValue( 0 );
}

void DynamicFaciesRelationshipDiagramDialog::onCapture()
{
    QClipboard *clipboard = QApplication::clipboard();
    const QPixmap px = *(ui->lblImage->pixmap()); //copy plot pixmap
    //TODO: this gives a flood of error messages when pixmap is not square.
    clipboard->setPixmap( px,  QClipboard::Clipboard );
    QMessageBox::information(this, "Plot snapshot", "The plot image has been copied to the clipboard. You can paste it to any application.");
}

void DynamicFaciesRelationshipDiagramDialog::onSave()
{
    int index = ui->sliderH->value();

    if( index >= m_images.size() ){
        Application::instance()->logWarn("DynamicFaciesRelationshipDiagramDialog::onSave(): invalid diagram index: " + QString::number( index ) );
        return;
    }

    //get the current pair h-Facies Transition Matrix
    hFTM hftm = m_hFTMs[ index ];

    //generate the diagram as PostScript file
    QString psFilePath;
    Util::makeFaciesRelationShipDiagramPlot( hftm.second,
                                             psFilePath,
                                             ui->dblSpinCutoff->value(),
                                             ui->chkMakeProportional->isChecked(),
                                             ui->spinPrecision->value(),
                                             ui->spinMaxLineThickness->value() );


    //ask the user to enter a name for the new project file.
    bool ok;
    QString proposed_name = "FRD_h_";
    proposed_name += QString::number( hftm.first );
    QString new_plot_name = QInputDialog::getText(this, "Name the new plot file",
                                             "New plot file name:", QLineEdit::Normal,
                                             proposed_name, &ok);

    //if the user didn't cancel, import the PostScript file as new plot object in the project
    if (ok && !new_plot_name.isEmpty()){
        Application::instance()->getProject()->importPlot( psFilePath, new_plot_name.append(".ps") );
    }
}
