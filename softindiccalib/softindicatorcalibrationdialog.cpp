#include "softindicatorcalibrationdialog.h"
#include "ui_softindicatorcalibrationdialog.h"

#include "domain/application.h"
#include "domain/attribute.h"
#include "domain/file.h"
#include "widgets/fileselectorwidget.h"
#include "softindicatorcalibplot.h"
#include "softindicatorcalibcanvaspicker.h"

#include <QHBoxLayout>

SoftIndicatorCalibrationDialog::SoftIndicatorCalibrationDialog(Attribute *at, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SoftIndicatorCalibrationDialog),
    m_at( at ),
    m_softIndCalibPlot( nullptr )
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

    //add the widget used to edit the calibration curves
    m_softIndCalibPlot = new SoftIndicatorCalibPlot(this);
    ui->frmCalib->layout()->addWidget( m_softIndCalibPlot );

    // The canvas picker handles all mouse and key
    // events on the plot canvas
    ( void ) new SoftIndicatorCalibCanvasPicker( m_softIndCalibPlot );

    adjustSize();
}

SoftIndicatorCalibrationDialog::~SoftIndicatorCalibrationDialog()
{
    delete ui;
    Application::instance()->logInfo("SoftIndicatorCalibrationDialog destroyed.");
}
