#include "faciesrelationshipdiagramdialog.h"
#include "ui_faciesrelationshipdiagramdialog.h"
#include "domain/faciestransitionmatrix.h"
#include "domain/application.h"
#include "domain/project.h"
#include "dialogs/displayplotdialog.h"
#include "graphviz/graphviz.h"

#include <QMessageBox>
#include <QStringBuilder>
#include <QTextStream>
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

    // make a script following the DOT syntax able to be parsed by GraphViz
    QString outputDOT = "digraph{\n";
    for( int i = 0; i < m_faciesTransitionMatrix->getRowCount(); ++i ){
        for( int j = 0; j < m_faciesTransitionMatrix->getColumnCount(); ++j ){
            // Help about the DOT style language: https://graphviz.gitlab.io/_pages/pdf/dotguide.pdf
            // Help about GraphViz API:           https://graphviz.gitlab.io/_pages/pdf/libguide.pdf
            // GraphViz general documentation:    https://www.graphviz.org/documentation/
            double diff = m_faciesTransitionMatrix->getDifference( i, j );
            if( diff > cutoff ){
                //style for the "from" facies
                QColor color = m_faciesTransitionMatrix->getColorOfCategoryInRowHeader( i ).lighter();
                QString rgb = QString::number( color.hueF() )   + " " +
                        QString::number( color.saturationF() ) + " " +
                        QString::number( color.lightnessF() )        ;
                QString labelColor = "black";
                if( color.lightnessF() < 0.6 ) //if the facies color is too dark, use white letters for the labels
                    labelColor = "white";
                outputDOT = outputDOT % "\"" % m_faciesTransitionMatrix->getRowHeader(i) % "\" [shape=box,style=filled,color=\"" % rgb % "\"," %
                        "label=<<FONT COLOR=\"" % labelColor % "\">" % m_faciesTransitionMatrix->getRowHeader(i) % "</FONT>>]\n";
                //style for the "to" facies
                color = m_faciesTransitionMatrix->getColorOfCategoryInColumnHeader( j );
                rgb = QString::number( color.hueF() )   + " " +
                        QString::number( color.saturationF() ) + " " +
                        QString::number( color.lightnessF() )        ;
                labelColor = "black";
                if( color.lightnessF() < 0.6 ) //if the facies color is too dark, use white letters for the labels
                    labelColor = "white";
                outputDOT = outputDOT % "\"" % m_faciesTransitionMatrix->getColumnHeader(j) % "\" [shape=box,style=filled,color=\"" % rgb % "\"," %
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

    //create a .dot file in the temporary directory
    QString dotFilePath = Application::instance()->getProject()->generateUniqueTmpFilePath("dot");

    //open the file for output
    QFile outputDotFile( dotFilePath );
    outputDotFile.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&outputDotFile);

    //write out dot syntax
    out << outputDOT << '\n';

    outputDotFile.close();

    //make a tmp PostScript file
    QString psFilePath = Application::instance()->getProject()->generateUniqueTmpFilePath("ps");

    //parse the dot file and render a PostScript file
    GraphViz::makePSfromDOT( dotFilePath, psFilePath );

    //display the PS file.
    DisplayPlotDialog* dpg = new DisplayPlotDialog( psFilePath, "Facies relationship diagram.", GSLibParameterFile(), this );
    dpg->show();
}
