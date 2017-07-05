#include "ndvestimationdialog.h"
#include "ui_ndvestimationdialog.h"

#include "domain/file.h"
#include "domain/attribute.h"
#include "domain/application.h"
#include "domain/cartesiangrid.h"
#include "domain/project.h"
#include "widgets/variogrammodelselector.h"
#include "geostats/gridcell.h"
#include "geostats/ndvestimation.h"
#include "util.h"

#include <QInputDialog>

NDVEstimationDialog::NDVEstimationDialog(Attribute *at, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NDVEstimationDialog),
    _at(at),
    _vmSelector(new VariogramModelSelector())
{
    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    ui->setupUi(this);

    setWindowTitle("Estimation of unvalued cells.");

    ui->lblGridName->setText( "<html><head/><body><p><span style=\" font-weight:600; color:#0000ff;\">" +
                              at->getContainingFile()->getName() + "</span></p></body></html>" );

    ui->lblVariableName->setText( "<html><head/><body><p><span style=\" font-weight:600; color:#0000ff;\">" +
                              at->getName() + "</span></p></body></html>" );

    ui->frmVariogramPlaceholder->layout()->addWidget( _vmSelector );

    updateMetricSizeLabels();
}

NDVEstimationDialog::~NDVEstimationDialog()
{
    Application::instance()->logInfo("NDVEstimationDialog destroyed.");
    delete ui;
}


void NDVEstimationDialog::updateMetricSizeLabels()
{
    //Assumes the partent file of the selected attribut is a Cartesian grid
    CartesianGrid *cg = (CartesianGrid*)_at->getContainingFile();

    QString text = "<html><head/><body><p><span style=\" font-weight:600; color:#0000ff;\">";
    text += QString::number( cg->getDX() * ui->spinNCols->value() ) + " X ";
    text += QString::number( cg->getDY() * ui->spinNRows->value() ) + " X ";
    text += QString::number( cg->getDZ() * ui->spinNSlices->value() );
    text += "</span></p></body></html>";

    ui->lblSizeMetric->setText( text );
}

void NDVEstimationDialog::run()
{
    //the parent file is surely a CartesianGrid.
    CartesianGrid *cg = (CartesianGrid*)_at->getContainingFile();

    //propose a name for the new grid to contain the estimates
    QString proposed_name = cg->getName() + "_NDV_Estimates.dat";

    //user enters the name for the new grid with the estimates
    QString new_cg_name = QInputDialog::getText(this, "Name the new grid",
                                             "Name for the grid with estimates:", QLineEdit::Normal,
                                             proposed_name );

    //if the user canceled the input box
    if ( new_cg_name.isEmpty() ){
        //abort
        return;
    }

    //run the estimation
    NDVEstimation* estimation = new NDVEstimation( _at );
    estimation->setSearchParameters( ui->spinNbSamples->value(),
                                     ui->spinNCols->value(),
                                     ui->spinNRows->value(),
                                     ui->spinNSlices->value());
    estimation->setVariogramModel( _vmSelector->getSelectedVModel() );
    estimation->setUseDefaultValue( ui->chkUseSKMeanAsDefault->isChecked() );
    estimation->setDefaultValue( ui->txtMeanForSK->text().toDouble() );
    estimation->setMeanForSK( ui->txtMeanForSK->text().toDouble() );
    if( ui->cmbKType->currentIndex() == 0 ) //label SK
        estimation->setKtype( KrigingType::SK );
    else
        estimation->setKtype( KrigingType::OK );
    std::vector<double> results = estimation->run();

    //make a tmp file path
    QString tmp_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");

    //crate a new cartesian grid pointing to the tmp path
    CartesianGrid * new_cg = new CartesianGrid( tmp_file_path );

    //set the geometry info based on the original grid
    new_cg->setInfoFromOtherCG( cg, false );

    //save the results in the project's tmp directory
    Util::createGEOEASGrid( _at->getName(), results, tmp_file_path);

    //import the saved file to the project
    Application::instance()->getProject()->importCartesianGrid( new_cg, new_cg_name );

}
