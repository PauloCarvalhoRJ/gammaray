#include "faciesrelationshipdiagramdialog.h"
#include "ui_faciesrelationshipdiagramdialog.h"
#include "domain/faciestransitionmatrix.h"
#include "domain/application.h"
#include "domain/project.h"
#include "dialogs/displayplotdialog.h"
#include "graphviz/graphviz.h"
#include "util.h"

#include <QMessageBox>
#include <QStringBuilder>
#include <QTextStream>
#include <QStringBuilder>
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

    QString psFilePath;
    Util::makeFaciesRelationShipDiagramPlot( *m_faciesTransitionMatrix,
                                             psFilePath,
                                             cutoff,
                                             ui->chkMakeLinesProportional->isChecked(),
                                             ui->spinPrecision->value(),
                                             ui->spinMaxLineWidth->value() );

    //display the PS file.
    DisplayPlotDialog* dpg = new DisplayPlotDialog( psFilePath, "Facies relationship diagram.", GSLibParameterFile(), this );
    dpg->show();
}
