#include "lvadatasetdialog.h"
#include "ui_lvadatasetdialog.h"

#include "domain/attribute.h"
#include "domain/file.h"
#include "domain/cartesiangrid.h"

LVADataSetDialog::LVADataSetDialog(Attribute* at, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LVADataSetDialog),
    m_at( at )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle( "Locally Varying Anisotropy Dialog" );

    ui->lblCaption->setText( "Locally varying anisotropy for [" + m_at->getContainingFile()->getName() + " / " + m_at->getName() + "]" );

    ui->lblGridName->setText( m_at->getContainingFile()->getName() );
}

LVADataSetDialog::~LVADataSetDialog()
{
    delete ui;
}

void LVADataSetDialog::updateSummary()
{
    QString summary;
    CartesianGrid* cg = dynamic_cast<CartesianGrid*>( m_at->getContainingFile() );
    if( ! cg ){
        std::stringstream ss;
        ss << "Grid is " << cg->getNX() << " x " << cg->getNY() << " x " << cg->getNZ();
        summary = QString( ss.str().c_str() );
    }
    else
        summary = "Input file is not a Cartesian grid.";
}

void LVADataSetDialog::onComputeLVA()
{

}
