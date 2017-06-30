#include "ndvestimationdialog.h"
#include "ui_ndvestimationdialog.h"

#include "domain/file.h"
#include "domain/attribute.h"
#include "domain/application.h"
#include "domain/cartesiangrid.h"
#include "widgets/variogrammodelselector.h"

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

}
