#include "gridrepositiondialog.h"
#include "ui_gridrepositiondialog.h"

#include "domain/cartesiangrid.h"

GridRepositionDialog::GridRepositionDialog(CartesianGrid *cg, QWidget *parent) :
    QDialog(parent),
    m_cg(cg),
    ui(new Ui::GridRepositionDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("Align/reposition grid");

    if( cg ){
        double x,y,z;

        ui->spinLLB_I->setRange(0, cg->getNI()-1 );
        ui->spinLLB_J->setRange(0, cg->getNJ()-1 );
        ui->spinLLB_K->setRange(0, cg->getNK()-1 );
        ui->spinLLB_I->setValue( 0 );
        ui->spinLLB_J->setValue( 0 );
        ui->spinLLB_K->setValue( 0 );

        ui->dblSpinLLB_X->setRange( -std::numeric_limits<double>::max(),  std::numeric_limits<double>::max() );
        ui->dblSpinLLB_Y->setRange( -std::numeric_limits<double>::max(),  std::numeric_limits<double>::max() );
        ui->dblSpinLLB_Z->setRange( -std::numeric_limits<double>::max(),  std::numeric_limits<double>::max() );
        cg->getCellLocation( 0, 0, 0, x, y, z );
        ui->dblSpinLLB_X->setValue( x );
        ui->dblSpinLLB_Y->setValue( y );
        ui->dblSpinLLB_Z->setValue( z );

        ui->spinURT_I->setRange(0, cg->getNI()-1 );
        ui->spinURT_J->setRange(0, cg->getNJ()-1 );
        ui->spinURT_K->setRange(0, cg->getNK()-1 );
        ui->spinURT_I->setValue( cg->getNI()-1 );
        ui->spinURT_J->setValue( cg->getNJ()-1 );
        ui->spinURT_K->setValue( cg->getNK()-1 );

        ui->dblSpinURT_X->setRange( -std::numeric_limits<double>::max(),  std::numeric_limits<double>::max() );
        ui->dblSpinURT_Y->setRange( -std::numeric_limits<double>::max(),  std::numeric_limits<double>::max() );
        ui->dblSpinURT_Z->setRange( -std::numeric_limits<double>::max(),  std::numeric_limits<double>::max() );
        cg->getCellLocation( cg->getNI()-1 , cg->getNJ()-1 , cg->getNK()-1 , x, y, z );
        ui->dblSpinURT_X->setValue( x );
        ui->dblSpinURT_Y->setValue( y );
        ui->dblSpinURT_Z->setValue( z );
    }
}

GridRepositionDialog::~GridRepositionDialog()
{
    delete ui;
}

void GridRepositionDialog::accept()
{
    m_cg->reposition( ui->spinLLB_I->value(), ui->spinLLB_J->value(), ui->spinLLB_K->value(),
                      ui->dblSpinLLB_X->value(),  ui->dblSpinLLB_Y->value(), ui->dblSpinLLB_Z->value(),
                      ui->spinURT_I->value(), ui->spinURT_J->value(), ui->spinURT_K->value(),
                      ui->dblSpinURT_X->value(),  ui->dblSpinURT_Y->value(), ui->dblSpinURT_Z->value());

    QDialog::accept();
}
