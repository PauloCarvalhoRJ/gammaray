#include "sisimdialog.h"
#include "ui_sisimdialog.h"

SisimDialog::SisimDialog(IKVariableType varType, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SisimDialog)
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    //configure UI captions according to IK variable type
    this->setWindowTitle( "Sequential Indicator Simulation for a " );
    if( varType == IKVariableType::CONTINUOUS ){
        this->setWindowTitle( windowTitle() + "continuous variable." );
        ui->lblIKVarType->setText("<html><head/><body><p><span style=\" font-weight:600; color:#0000ff;\">CONTINUOUS</span></p></body></html>");
        ui->lblDistributionFile->setText("Threshold c.d.f. file:");
    } else {
        this->setWindowTitle( windowTitle() + "categorical variable." );
        ui->lblIKVarType->setText("<html><head/><body><p><span style=\" font-weight:600; color:#0000ff;\">CATEGORICAL</span></p></body></html>");
        ui->lblDistributionFile->setText("Category p.d.f. file:");
    }
}

SisimDialog::~SisimDialog()
{
    delete ui;
}
