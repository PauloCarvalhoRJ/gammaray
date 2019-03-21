#include "dynamicfaciesrelationshipdiagramdialog.h"
#include "ui_dynamicfaciesrelationshipdiagramdialog.h"

#include "gs/ghostscript.h"
#include "exceptions/externalprogramexception.h"
#include "domain/application.h"
#include "domain/project.h"

#include <QMessageBox>

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

    setWindowTitle("Dynamic Facies Relationship Dialog");

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    //compute the FTMs for all separations
    m_hFTMs = Util::computeFaciesTransitionMatrices( m_categoricalAttributes,
                                                     m_hInitial,
                                                     m_hFinal,
                                                     m_nSteps,
                                                     m_toleranceCoefficient );

    //compress the FTMs
    Util::compressFaciesTransitionMatrices( m_hFTMs );

    //for each FTM
    for( hFTM& hftm : m_hFTMs ){

        //plot a Facies Relationship Diagram for a FTM as a Postscript file
        QString psFilePath;
        Util::makeFaciesRelationShipDiagramPlot( hftm.second,
                                                 psFilePath,
                                                 0.05,
                                                 true,
                                                 1,
                                                 10 );

        //call Ghostscript to parse the PS file and render it as a PNG image file.
        try{
            QString png_file_name = Application::instance()->getProject()->generateUniqueTmpFilePath("png");
            Ghostscript::makePNG( psFilePath, png_file_name, 80 );
        } catch ( ExternalProgramException &ex ){
            QMessageBox::critical(this, "GhostScript Error", QString("Failed to run Ghostscript: error code=").append( QString::number(ex.code) ));
        }

    }

}

DynamicFaciesRelationshipDiagramDialog::~DynamicFaciesRelationshipDiagramDialog()
{
    delete ui;
}
