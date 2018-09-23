#include "variograminputdialog.h"
#include "ui_variograminputdialog.h"
#include "widgets/variogrammodellist.h"
#include "domain/variogrammodel.h"

VariogramInputDialog::VariogramInputDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VariogramInputDialog)
{
    ui->setupUi(this);

    setWindowTitle("Select variogram model.");

    ui->lblCaption->hide();

    m_variogramList = new VariogramModelList();
    ui->frmVariograms->layout()->addWidget( m_variogramList );
}

VariogramInputDialog::~VariogramInputDialog()
{
    delete ui;
}

VariogramModel *VariogramInputDialog::getSelectedVariogramModel()
{
    return m_variogramList->getSelectedVModel();
}

void VariogramInputDialog::setCaption(QString caption)
{
    ui->lblCaption->setText( caption );
    ui->lblCaption->show();
}
