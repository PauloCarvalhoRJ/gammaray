#include "imagejockeydialog.h"
#include "ui_imagejockeydialog.h"
#include "widgets/cartesiangridselector.h"
#include "widgets/variableselector.h"
#include "widgets/grcompass.h"
#include "domain/application.h"
#include "domain/cartesiangrid.h"
#include "domain/attribute.h"
#include "imagejockeygridplot.h"
#include "spectrogram1dparameters.h"
#include "spectrogram1dplot.h"

#include <qwt_wheel.h>

ImageJockeyDialog::ImageJockeyDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImageJockeyDialog),
    m_spectrogram1Dparams( new Spectrogram1DParameters() )
{
    ui->setupUi(this);

    setWindowTitle( "Image Jockey" );

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    //maximizes the dialog
    this->setWindowFlag( Qt::WindowMaximizeButtonHint );

    //the grid plot widget (displayed in the left pane)
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
    m_wheelColorDecibelReference->setToolTip("0dB reference level");
    //connects the wheel widgets events to the grid plot widget so the formers can be used to control the latter
    connect( m_wheelColorMax, SIGNAL(valueChanged(double)), m_gridPlot, SLOT(setColorScaleMax(double)) );
    connect( m_wheelColorMin, SIGNAL(valueChanged(double)), m_gridPlot, SLOT(setColorScaleMin(double)) );
    connect( m_wheelColorDecibelReference, SIGNAL(valueChanged(double)),
             m_gridPlot, SLOT(setDecibelRefValue(double)));

    //these widgets control the band used to get a 1D spectrogram from the 2D spectrogram.
    m_azimuthCompass = new GRCompass(2);
    ui->frmAzimuthControl->layout()->addWidget( m_azimuthCompass );
    m_azimthTolControl = new QwtWheel();
    ui->frmAzimuthTolControl->layout()->addWidget( m_azimthTolControl );
    m_bandwidthControl = new QwtWheel();
    ui->frmBandwidthControl->layout()->addWidget( m_bandwidthControl );
    m_radiusControl = new QwtWheel();
    ui->frmRadiusControl->layout()->addWidget( m_radiusControl );
    //connect the controls' events to the 1D spectrogram calculation object so the user can control it
    connect( m_azimuthCompass, SIGNAL(valueChanged(double)), m_spectrogram1Dparams, SLOT(setAzimuth(double)) );
    connect( m_azimthTolControl, SIGNAL(valueChanged(double)), m_spectrogram1Dparams, SLOT(setAzimuthTolerance(double)) );
    connect( m_bandwidthControl, SIGNAL(valueChanged(double)), m_spectrogram1Dparams, SLOT(setBandWidth(double)) );
    connect( m_radiusControl, SIGNAL(valueChanged(double)), m_spectrogram1Dparams, SLOT(setRadius(double)) );

    //set some default parameters for the 1D spectrogram calculation band
    m_spectrogram1Dparams->setAzimuth( 0.0 );
    m_spectrogram1Dparams->setAzimuthTolerance( 22.5 );
    m_spectrogram1Dparams->setBandWidth( 3000.0 );
    m_spectrogram1Dparams->setRadius( 0.0 );

    //the 1D spectrogram plot
    m_spectrogram1Dplot = new Spectrogram1DPlot();
    ui->frm1DSpectrogram->layout()->addWidget( m_spectrogram1Dplot );
    ui->frm1DSpectrogram->setStyleSheet("background-color: black; border-radius: 10px; color: #00FF00;");
    connect( m_wheelColorDecibelReference, SIGNAL(valueChanged(double)),
             m_spectrogram1Dplot, SLOT(setDecibelRefValue(double)));
    connect( m_wheelColorMax, SIGNAL(valueChanged(double)), m_spectrogram1Dplot, SLOT(setVerticalScaleMax(double)) );
    connect( m_wheelColorMin, SIGNAL(valueChanged(double)), m_spectrogram1Dplot, SLOT(setVerticalScaleMin(double)) );

    //update the visual representation of the 1D spectrogram calculation band whenever one of its
    //paramaters (e.g. azimuth) changes.
    connect( m_spectrogram1Dparams, SIGNAL(updated()), m_gridPlot, SLOT( draw1DSpectrogramBand()) );

    //update the 1D spectrogram plot whenever the 1D spectrogram calculation band changes one of its
    //paramaters (e.g. azimuth).
    connect( m_spectrogram1Dparams, SIGNAL(updated()), m_spectrogram1Dplot, SLOT( rereadSpectrogramData()) );

    //calling this slot causes the variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_cgSelector->onSelection( 0 );
}

ImageJockeyDialog::~ImageJockeyDialog()
{
    delete ui;
    delete m_spectrogram1Dparams;
    Application::instance()->logInfo("ImageJockeyDialog destroyed.");
}

void ImageJockeyDialog::onUpdateGridPlot(Attribute *at)
{
    //set the attribute
    m_gridPlot->setAttribute( at );

    //read decibel extrema before assigning them to the widgets to prevent
    //that some unpredicted signal/slot chaining causes unpredicted behavior
    double dBmin = m_gridPlot->getScaleMinValue();
    double dBmax = m_gridPlot->getScaleMaxValue();
    double dBstep = (dBmax-dBmin) / 360.0d; //dividing the span into units per wheel degree

    //setup the color scale wheel widgets
    m_wheelColorMax->setMaximum( dBmax );
    m_wheelColorMax->setMinimum( dBmin );
    m_wheelColorMax->setSingleStep( dBstep );
    m_wheelColorMax->setValue( dBmax );
    m_wheelColorMin->setMaximum( dBmax );
    m_wheelColorMin->setMinimum( dBmin );
    m_wheelColorMin->setValue( dBmin );
    m_wheelColorMin->setSingleStep( dBstep );

    //setup the dB reference wheel widget
    int columnIndex = at->getAttributeGEOEASgivenIndex() - 1;
    CartesianGrid* cg = (CartesianGrid*)at->getContainingFile(); //assumes Attribute's parent file is a Cartesian grid
    cg->loadData();
    double dataAbsMin = cg->minAbs( columnIndex );
    double dataAbsMax = cg->maxAbs( columnIndex );
    double dataAbsStep = (dataAbsMax - dataAbsMin) / 360.0d; //dividing the span into units per wheel degree
    m_wheelColorDecibelReference->setMaximum( dataAbsMax );
    m_wheelColorDecibelReference->setMinimum( dataAbsMin );
    m_wheelColorDecibelReference->setSingleStep( dataAbsStep );
    m_wheelColorDecibelReference->setValue( (dataAbsMax + dataAbsMin) / 2.0d ); //set the 0dB reference in the middle

    //setup the 1D spectrogram calculation band controls
    double gridDiagLength = cg->getDiagonalLength();
    m_bandwidthControl->setMaximum( gridDiagLength / 10.0d );
    m_bandwidthControl->setMinimum( 0.0d );
    m_bandwidthControl->setValue( m_bandwidthControl->maximum() / 2.0d );
    m_bandwidthControl->setSingleStep( m_bandwidthControl->maximum() / 360.0d );
    m_radiusControl->setMaximum( gridDiagLength / 2.0d );
    m_radiusControl->setMinimum( 0.0d );
    m_radiusControl->setValue( 0.0d );
    m_radiusControl->setSingleStep( m_radiusControl->maximum() / 360.0d );
    m_azimthTolControl->setMinimum( 10.0d ); //10 degrees
    m_azimthTolControl->setMaximum( 90.0d ); //90 degrees
    m_azimthTolControl->setValue( 45.0d );
    m_azimthTolControl->setSingleStep( (m_azimthTolControl->maximum()-m_azimthTolControl->minimum()) / 360.0d );
    m_azimuthCompass->setValue( 0.0d ); //N000E (north)

    //the length of a half band for the 1D spectrogram calculation is half the diagonal of the grid
    //this ensures total grid coverage regardless of azimuth choice
    m_spectrogram1Dparams->setEndRadius( gridDiagLength / 2.0d );
    m_spectrogram1Dparams->setRefCenter( cg->getCenter() );

    //set the attribute for the 1D spectrogram plot
    m_spectrogram1Dplot->setAttribute( at );

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


