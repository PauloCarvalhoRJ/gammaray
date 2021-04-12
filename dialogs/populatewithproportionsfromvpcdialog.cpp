#include "populatewithproportionsfromvpcdialog.h"
#include "ui_populatewithproportionsfromvpcdialog.h"

#include "domain/verticalproportioncurve.h"
#include "domain/cartesiangrid.h"

PopulateWithProportionsFromVPCDialog::PopulateWithProportionsFromVPCDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PopulateWithProportionsFromVPCDialog),
    m_cg( nullptr ),
    m_vpc( nullptr )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle( "Populate object with proportions from a VPC" );
}

PopulateWithProportionsFromVPCDialog::~PopulateWithProportionsFromVPCDialog()
{
    delete ui;
}

void PopulateWithProportionsFromVPCDialog::setCartesianGrid(CartesianGrid *cg)
{
    m_cg = cg;
    updateInterface();
}

void PopulateWithProportionsFromVPCDialog::setVPC(VerticalProportionCurve *vpc)
{
    m_vpc = vpc;
    updateInterface();
}

void PopulateWithProportionsFromVPCDialog::updateInterface()
{
    ui->lblVPCname->setText( "" );
    ui->lblTargetObjectName->setText( "" );
    ui->spinTopK->setRange( 0, 99 );
    ui->spinTopK->setValue( 0 );
    ui->spinBaseK->setRange( 0, 99 );
    ui->spinBaseK->setValue( 0 );

    if( m_vpc ){
        ui->lblVPCname->setText( m_vpc->getName() );
    }

    if( m_cg ){
        ui->lblTargetObjectName->setText( m_cg->getName() );
        ui->spinTopK->setRange( 0, m_cg->getNK()-1 );
        ui->spinTopK->setValue( m_cg->getNK()-1 );
        ui->spinBaseK->setRange( 0, m_cg->getNK()-1 );
        ui->spinBaseK->setValue( 0 );
    }
}

void PopulateWithProportionsFromVPCDialog::onProcess()
{
    uint baseK = ui->spinBaseK->value();
    uint topK = ui->spinTopK->value();

    m_vpc->readFromFS();

    m_cg->fillWithProportions( m_vpc, baseK, topK );
}
