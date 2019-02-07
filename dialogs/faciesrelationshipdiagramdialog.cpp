#include "faciesrelationshipdiagramdialog.h"
#include "ui_faciesrelationshipdiagramdialog.h"
#include "domain/faciestransitionmatrix.h"
#include "domain/application.h"
#include "domain/project.h"
#include "dialogs/displayplotdialog.h"

#include <QMessageBox>
#include <QStringBuilder>
#include <gvc.h>
#include <iostream>

FaciesRelationShipDiagramDialog::FaciesRelationShipDiagramDialog(FaciesTransitionMatrix *ftm, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FaciesRelationShipDiagramDialog),
    m_faciesTransitionMatrix( ftm )
{
    ui->setupUi(this);

    setWindowTitle("Facies Relationship Diagram");

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    m_faciesTransitionMatrix->readFromFS();

    if( m_faciesTransitionMatrix->isUsable() ){
        performCalculation();
    } else {
        QMessageBox::critical( this, "Error", "The matrix is not usable.  You need to associate it to a category definition or"
                                              " some facies names were not found in the associated category definition.\n"
                                              " Please refer to the program manual for forther details. ");
    }
}

FaciesRelationShipDiagramDialog::~FaciesRelationShipDiagramDialog()
{
    delete ui;
}

void FaciesRelationShipDiagramDialog::performCalculation()
{
    double cutoff = ui->dblSpinCutoff->value();
    Agraph_t* G;
    GVC_t* gvc;

    //----------------------------------processing with GraphViz--------------------------------------------------
    //create a GVC library context
    gvc = gvContext();

    { //G = createGraph ();
        QString outputDOT = "digraph{\n";
        for( int i = 0; i < m_faciesTransitionMatrix->getRowCount(); ++i ){
            for( int j = 0; j < m_faciesTransitionMatrix->getColumnCount(); ++j ){
                // Help about the DOT style language: https://graphviz.gitlab.io/_pages/pdf/dotguide.pdf
                // Help about GraphViz API:           https://graphviz.gitlab.io/_pages/pdf/libguide.pdf
                // GraphViz general documentation:    https://www.graphviz.org/documentation/
                double diff = m_faciesTransitionMatrix->getDifference( i, j );
                if( diff > cutoff ){
                    //style for the "from" facies
                    QColor color = m_faciesTransitionMatrix->getColorOfCategoryInRowHeader( i ).toHsv();
                    double hue = color.hueF();
                    if( hue < 0 )
                        hue *= -1.0;
                    QString hsv = QString::number( hue )   + " " +
                                  QString::number( color.saturationF() ) + " " +
                                  QString::number( color.valueF() )        ;
                    QString labelColor = "black";
                    if( color.lightnessF() < 0.6 ) //if the facies color is too dark, use white letters for the labels
                        labelColor = "white";
                    outputDOT = outputDOT % "\"" % m_faciesTransitionMatrix->getRowHeader(i) % "\" [shape=box,style=filled,color=\"" % hsv % "\"," %
                                          "label=<<FONT COLOR=\"" % labelColor % "\">" % m_faciesTransitionMatrix->getRowHeader(i) % "</FONT>>]\n";
                    //style for the "to" facies
                    color = m_faciesTransitionMatrix->getColorOfCategoryInColumnHeader( j ).toHsv();
                    hue = color.hueF();
                    if( hue < 0 )
                        hue *= -1.0;
                    hsv = QString::number( hue )   + " " +
                          QString::number( color.saturationF() ) + " " +
                          QString::number( color.valueF() )        ;
                    labelColor = "black";
                    if( color.lightnessF() < 0.6 ) //if the facies color is too dark, use white letters for the labels
                        labelColor = "white";
                    outputDOT = outputDOT % "\"" % m_faciesTransitionMatrix->getColumnHeader(j) % "\" [shape=box,style=filled,color=\"" % hsv % "\"," %
                            "label=<<FONT COLOR=\"" % labelColor % "\">" % m_faciesTransitionMatrix->getColumnHeader(j) % "</FONT>>]\n";
                    //style for the edge connecting both facies
                    outputDOT = outputDOT % "\"" % m_faciesTransitionMatrix->getRowHeader(i) % "\" -> \"" %
                                                   m_faciesTransitionMatrix->getColumnHeader(j) % "\"" %
                                                   "[label=\"" % QString::number(diff,'g',ui->spinPrecision->value()) % "\"";
                    if( ui->chkMakeLinesProportional->isChecked() )
                        outputDOT = outputDOT % ",style=\"setlinewidth(" % QString::number((int)(diff*ui->spinMaxLineWidth->value())) % ")\"";
                    outputDOT = outputDOT % "]\n";
                }
            }
        }
        outputDOT = outputDOT % "}\n";

        //std::cout << outputDOT.toStdString() << std::endl;

        G = agmemread( outputDOT.toStdString().c_str() );
    }

    //layout the graph
    gvLayout (gvc, G, "dot");

    //make a tmp PostScript file
    QString psFilePath = Application::instance()->getProject()->generateUniqueTmpFilePath("ps");

    { //drawGraph (G) to some output device;
        const char* format = "ps";
        gvRenderFilename( gvc, G, format, psFilePath.toStdString().c_str() );
    }

    //free resources
    gvFreeLayout(gvc, G);
    agclose (G);
    gvFreeContext(gvc);
    //-------------------------------------------end of processing with GraphViz--------------------------------------

    DisplayPlotDialog* dpg = new DisplayPlotDialog( psFilePath, "Facies relationship diagram.", GSLibParameterFile(), this );
    dpg->show();
}
