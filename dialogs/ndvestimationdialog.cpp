#include "ndvestimationdialog.h"
#include "ui_ndvestimationdialog.h"

#include "domain/file.h"
#include "domain/attribute.h"
#include "domain/application.h"
#include "domain/cartesiangrid.h"
#include "widgets/variogrammodelselector.h"
#include "geostats/gridcell.h"

NDVEstimationDialog::NDVEstimationDialog(Attribute *at, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NDVEstimationDialog),
    _at(at)
{
    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    ui->setupUi(this);

    setWindowTitle("Estimation of unvalued cells.");

    ui->lblGridName->setText( "<html><head/><body><p><span style=\" font-weight:600; color:#0000ff;\">" +
                              at->getContainingFile()->getName() + "</span></p></body></html>" );

    ui->lblVariableName->setText( "<html><head/><body><p><span style=\" font-weight:600; color:#0000ff;\">" +
                              at->getName() + "</span></p></body></html>" );

    ui->frmVariogramPlaceholder->layout()->addWidget( new VariogramModelSelector() );

    updateMetricSizeLabels();
}

NDVEstimationDialog::~NDVEstimationDialog()
{
    Application::instance()->logInfo("NDVEstimationDialog destroyed.");
    delete ui;
}

void NDVEstimationDialog::krige(GridCell cell)
{
    //Assumes the partent file of the selected attribut is a Cartesian grid
    CartesianGrid *cg = (CartesianGrid*)_at->getContainingFile();

    //collects valued neighbors
    std::vector<GridCell> vCells = cg->getValuedNeighbors( cell,
                                                           ui->spinNbSamples->value(),
                                                           ui->spinNCols->value(),
                                                           ui->spinNRows->value(),
                                                           ui->spinNSlices->value() );
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
    //Assumes the partent file of the selected attribut is a Cartesian grid
    CartesianGrid *cg = (CartesianGrid*)_at->getContainingFile();

    //gets the Attribute's column in its Cartesian grid's data array (GEO-EAS index - 1)
    uint atIndex = _at->getAttributeGEOEASgivenIndex() - 1;

    uint nI = cg->getNX();
    uint nJ = cg->getNY();
    uint nK = cg->getNZ();

    for( uint k = 0; k <nK; ++k)
        for( uint j = 0; j <nJ; ++j)
            for( uint i = 0; i <nI; ++i){
                double value = cg->dataIJK( atIndex, i, j, k );
                if( cg->isNDV( value ) ){
                    krige( GridCell(atIndex, i,j,k) );
                }
            }
}
