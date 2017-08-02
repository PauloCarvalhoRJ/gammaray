#include "imagejockeydialog.h"
#include "ui_imagejockeydialog.h"
#include "widgets/cartesiangridselector.h"
#include "widgets/variableselector.h"
#include "domain/application.h"
#include "imagejockeygridplot.h"

#include <qwt_wheel.h>

ImageJockeyDialog::ImageJockeyDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImageJockeyDialog)
{
    ui->setupUi(this);

    setWindowTitle( "Image Jockey" );

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowFlag( Qt::WindowMaximizeButtonHint );

    m_gridPlot = new ImageJockeyGridPlot();
    ui->frmGridPlot->layout()->addWidget( m_gridPlot );

    //the combo box to choose a Cartesian grid containing a Fourier image
    m_cgSelector = new CartesianGridSelector( true );
    ui->frmCmbGridPlaceholder->layout()->addWidget( m_cgSelector );

    //the combo box to choose the variable with the real part
    m_atSelector = new VariableSelector();
    ui->frmCmbAttributePlaceholder->layout()->addWidget( m_atSelector );
    connect( m_cgSelector, SIGNAL(cartesianGridSelected(DataFile*)),
             m_atSelector, SLOT(onListVariables(DataFile*)) );
    connect( m_atSelector, SIGNAL(variableSelected(Attribute*)),
             this, SLOT(onUpdateGridPlot(Attribute*)));

    //these wheels control the visual scale in decibels (dB)
    m_wheelColorMax = new QwtWheel();
    m_wheelColorMax->setOrientation( Qt::Vertical );
    ui->frmScaleMaxCtrlPlace->layout()->addWidget( m_wheelColorMax );
    m_wheelColorMin = new QwtWheel();
    m_wheelColorMin->setOrientation( Qt::Vertical );
    ui->frmScaleMinCtrlPlace->layout()->addWidget( m_wheelColorMin );
    m_wheelColorDecibelReference = new QwtWheel();
    m_wheelColorDecibelReference->setOrientation( Qt::Vertical );
    ui->frmDecibelRefCtrlPlace->layout()->addWidget( m_wheelColorDecibelReference );

    //calling this slot causes the variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_cgSelector->onSelection( 0 );

    //connects the wheels events to the plot so they can be used to control the plot
    connect( m_wheelColorMax, SIGNAL(valueChanged(double)), m_gridPlot, SLOT(setColorScaleMax(double)) );
    connect( m_wheelColorMin, SIGNAL(valueChanged(double)), m_gridPlot, SLOT(setColorScaleMin(double)) );
}

ImageJockeyDialog::~ImageJockeyDialog()
{
    delete ui;
    Application::instance()->logInfo("ImageJockeyDialog destroyed.");
}

void ImageJockeyDialog::onUpdateGridPlot(Attribute *at)
{
    m_gridPlot->setAttribute( at );

    m_wheelColorMax->setMaximum( m_gridPlot->getScaleMaxValue() );
    m_wheelColorMax->setMinimum( m_gridPlot->getScaleMinValue() );
    m_wheelColorMin->setMaximum( m_gridPlot->getScaleMaxValue() );
    m_wheelColorMin->setMinimum( m_gridPlot->getScaleMinValue() );
    m_wheelColorMax->setValue( m_wheelColorMax->maximum() );
    m_wheelColorMin->setValue( m_wheelColorMin->minimum() );

    //Perturb the splitter to force a redraw.
    //TODO: find out a more elegant way to make the Qwt Plot redraw (replot() is not working in setAttribute())
//    {
//        QList<int> oldSizes = ui->splitter->sizes();
//        QList<int> tmpSizes = oldSizes;
//        tmpSizes[0] = oldSizes[0] + 1;
//        tmpSizes[1] = oldSizes[1] - 1;
//        ui->splitter->setSizes( tmpSizes );
//        qApp->processEvents();
//        ui->splitter->setSizes( oldSizes );
    //    }
}


