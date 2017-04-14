#include "softindicatorcalibrationdialog.h"
#include "ui_softindicatorcalibrationdialog.h"

#include "domain/application.h"
#include "domain/attribute.h"
#include "domain/file.h"
#include "widgets/fileselectorwidget.h"
#include "softindicatorcalibplot.h"

#include <QHBoxLayout>

SoftIndicatorCalibrationDialog::SoftIndicatorCalibrationDialog(Attribute *at, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SoftIndicatorCalibrationDialog),
    m_at( at )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle("Soft indicator calibration for " + at->getContainingFile()->getName() + "/" + at->getName());

    //add a category definition file selection drop down menu
    m_fsw = new FileSelectorWidget( FileSelectorType::CategoryDefinitions, true );
    ui->frmTopBar->layout()->addWidget( m_fsw );

    //add a spacer for better layout
    QHBoxLayout *hl = (QHBoxLayout*)(ui->frmTopBar->layout());
    hl->addStretch();

    ui->frmCalib->layout()->addWidget( new SoftIndicatorCalibPlot(this) );

    adjustSize();
}

SoftIndicatorCalibrationDialog::~SoftIndicatorCalibrationDialog()
{
    delete ui;
    Application::instance()->logInfo("SoftIndicatorCalibrationDialog destroyed.");
}
