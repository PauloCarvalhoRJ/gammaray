#include "subgriddialog.h"
#include "ui_subgriddialog.h"

#include "domain/cartesiangrid.h"
#include "domain/application.h"
#include "domain/project.h"
#include "imagejockey/widgets/ijquick3dviewer.h"
#include "widgets/variableselector.h"

SubgridDialog::SubgridDialog(CartesianGrid *cg, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SubgridDialog),
    m_cg( cg )
{
    assert( cg && "SubgridDialog::SubgridDialog(): Cartesian grid cannot be null." );

    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle( "Make subgrid" );

    ui->spinMaxI->setMinimum( 0 );
    ui->spinMaxI->setMaximum( cg->getNI()-1 );
    ui->spinMaxI->setValue( cg->getNI()-1 );

    ui->spinMinI->setMinimum( 0 );
    ui->spinMinI->setMaximum( cg->getNI()-1 );
    ui->spinMinI->setValue( 0 );

    ui->spinMaxJ->setMinimum( 0 );
    ui->spinMaxJ->setMaximum( cg->getNJ()-1 );
    ui->spinMaxJ->setValue( cg->getNJ()-1 );

    ui->spinMinJ->setMinimum( 0 );
    ui->spinMinJ->setMaximum( cg->getNJ()-1 );
    ui->spinMinJ->setValue( 0 );

    ui->spinMaxK->setMinimum( 0 );
    ui->spinMaxK->setMaximum( cg->getNK()-1 );
    ui->spinMaxK->setValue( cg->getNK()-1 );

    ui->spinMinK->setMinimum( 0 );
    ui->spinMinK->setMaximum( cg->getNK()-1 );
    ui->spinMinK->setValue( 0 );

    ui->txtGridName->setText( cg->getName() + "_subgrid" );

    m_previewWidget = new IJQuick3DViewer();
    ui->frmDisplay->layout()->addWidget( m_previewWidget );
    m_previewWidget->hideDismissButton();

    //The list with the secondary data grid variables;
    m_variableForPreview = new VariableSelector();
    ui->frmCmbVariable->layout()->addWidget( m_variableForPreview );
    m_variableForPreview->onListVariables( m_cg );

    adjustSize();
}

SubgridDialog::~SubgridDialog()
{
    delete ui;
}

void SubgridDialog::onSave()
{
    CartesianGrid* subgrid = m_cg->makeSubGrid( ui->spinMinI->value(), ui->spinMaxI->value(),
                                                ui->spinMinJ->value(), ui->spinMaxJ->value(),
                                                ui->spinMinK->value(), ui->spinMaxK->value() );

    //save the physical files
    subgrid->setPath( Application::instance()->getProject()->getPath() +
                                      '/' + ui->txtGridName->text() );
    subgrid->updateMetaDataFile();
    subgrid->writeToFS();

    //adds the new grid to the project
    Application::instance()->getProject()->addDataFile( subgrid );
    Application::instance()->refreshProjectTree();

    //closes the dialog
    accept();
}

void SubgridDialog::onPreview()
{
    CartesianGrid* subgrid = m_cg->makeSubGrid( ui->spinMinI->value(), ui->spinMaxI->value(),
                                                ui->spinMinJ->value(), ui->spinMaxJ->value(),
                                                ui->spinMinK->value(), ui->spinMaxK->value() );

    uint dataColumn = m_variableForPreview->getSelectedVariableGEOEASIndex() - 1;

    spectral::array* data = subgrid->createSpectralArray( dataColumn );

    double min = subgrid->min( dataColumn );
    double max = subgrid->max( dataColumn );

    m_previewWidget->display( *data, min, max, subgrid->getDX(), subgrid->getDY(), subgrid->getDZ() );

    delete data;
    delete subgrid;
}
