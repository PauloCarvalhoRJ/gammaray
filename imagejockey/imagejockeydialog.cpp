#include "imagejockeydialog.h"
#include "ui_imagejockeydialog.h"
#include "widgets/cartesiangridselector.h"
#include "widgets/variableselector.h"
#include "widgets/grcompass.h"
#include "domain/application.h"
#include "domain/cartesiangrid.h"
#include "domain/attribute.h"
#include "domain/project.h"
#include "imagejockeygridplot.h"
#include "spectrogram1dparameters.h"
#include "spectrogram1dplot.h"
#include "equalizer/equalizerwidget.h"
#include "util.h"
#include "svdparametersdialog.h"
#include "spectral/svd.h"

#include <QInputDialog>
#include <QMessageBox>
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
    Qt::WindowFlags flags = this->windowFlags();
    flags |= Qt::WindowMaximizeButtonHint;
    this->setWindowFlags( flags );

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

    //the combo box to choose the variable with the imaginary part
    m_atSelectorImag = new VariableSelector();
    ui->frmCmbImagPartPlaceholder->layout()->addWidget( m_atSelectorImag );
    connect( m_cgSelector, SIGNAL(cartesianGridSelected(DataFile*)),
             m_atSelectorImag, SLOT(onListVariables(DataFile*)) );

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

    //the set of sliders to attenuate or amplify frquency components.
    m_equalizerWidget = new EqualizerWidget();
    ui->frmEqualizer->layout()->addWidget( m_equalizerWidget );

    //update the two vertical lines in the 1D spectrogram plot that visually represent the frequency
    //window selected in the equalizer widget.
    connect( m_equalizerWidget, SIGNAL(frequencyWindowUpdated(double,double)),
             m_spectrogram1Dplot, SLOT(updateFrequencyWindow(double,double)) );

    //to be notified when the user adjusts the equalizer sliders
    connect( m_equalizerWidget, SIGNAL(equalizerAdjusted(double,double)),
             this, SLOT(equalizerAdjusted(double,double)) );

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

void ImageJockeyDialog::spectrogramGridReplot()
{
    //TODO: find out a more elegant way to make the Qwt Plot redraw (replot() is not working)
    QList<int> oldSizes = ui->splitter->sizes();
    QList<int> tmpSizes = oldSizes;
    tmpSizes[0] = oldSizes[0] + 1;
    tmpSizes[1] = oldSizes[1] - 1;
    ui->splitter->setSizes( tmpSizes );
    qApp->processEvents();
    ui->splitter->setSizes( oldSizes );
    qApp->processEvents();
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

    //assuming Fourier image symmetry, the frequency limits range between 0.0 (DC) and half grid size
    m_equalizerWidget->setFrequencyLimits(0.0d, gridDiagLength / 2.0d);

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

void ImageJockeyDialog::resetReferenceCurve()
{
    m_spectrogram1Dplot->resetReferenceCurve();
}

void ImageJockeyDialog::equalizerAdjusted(double centralFrequency, double delta_dB )
{
    //assuming the selected file is a Cartesian grid
    CartesianGrid* cg = (CartesianGrid*)m_cgSelector->getSelectedDataFile();
    if( ! cg )
        return;

    //get the variable
    Attribute* at = cg->getAttributeFromGEOEASIndex( m_atSelector->getSelectedVariableGEOEASIndex() );

    //get the geometry of the area of influence in the 2D spectrogram.
    //The central frequency is the distance from the center of the 2D spectrogram.
    //The resulting area is defined taking into account the azimuth, which is set
    //   in the Spectrogram1DParamaters object.
    QList<QPointF> aoi = m_spectrogram1Dparams->getAreaOfInfluence( centralFrequency, m_equalizerWidget->getFrequencyStep() );

    //get the half-band geometry, as the azimuth tolerance setting may cause the final selection
    //to clip the area-of-influence
    QList<QPointF> halfBand = m_spectrogram1Dparams->getHalfBandGeometry();

    //perform the equalization of values
    cg->equalizeValues( aoi, delta_dB, at->getAttributeGEOEASgivenIndex()-1, m_wheelColorDecibelReference->value(), halfBand );

    //mirror the area of influence about the center of the 2D spectrogram
    Util::mirror2D( aoi, cg->getCenter() );

    //mirror the half-band geometry about the center of the 2D spectrogram
    Util::mirror2D( halfBand, cg->getCenter() );

    //perform the equalization in the opposite area to preserve the 2D spectrogram's symmetry
    cg->equalizeValues( aoi, delta_dB, at->getAttributeGEOEASgivenIndex()-1, m_wheelColorDecibelReference->value(), halfBand );

    //update the 2D spectrogram plot
    spectrogramGridReplot();

    //causes an update in the 1D spectrogram widget
    //TODO: this is not very elegant
    m_spectrogram1Dparams->setAzimuth( m_spectrogram1Dparams->azimuth() );
}

void ImageJockeyDialog::save()
{
    //assuming the selected file is a Cartesian grid
    CartesianGrid* cg = (CartesianGrid*)m_cgSelector->getSelectedDataFile();
    if( ! cg )
        return;

    //save edited Fourier image to filesystem
    cg->writeToFS();

    Application::instance()->logInfo("ImageJockeyDialog::save(): file saved.");
}

void ImageJockeyDialog::preview()
{
    //assuming the selected file is a Cartesian grid
    CartesianGrid* cg = (CartesianGrid*)m_cgSelector->getSelectedDataFile();
    if( ! cg )
        return;

    //creates a tmp path for the Cartesian grid with the FFT image
    QString tmpPathFFT = Application::instance()->getProject()->generateUniqueTmpFilePath( "dat" );

    //create a Cartesian grid object pointing to the newly created file
    CartesianGrid* cgFFTtmp = new CartesianGrid( tmpPathFFT );

    //get the gridspecs from the original FFT image
    cgFFTtmp->setInfoFromOtherCG( cg );

    //get the edited Fourier data
    std::vector<std::complex<double> > data =
         cg->getArray( m_atSelector->getSelectedVariableGEOEASIndex()-1,
                       m_atSelectorImag->getSelectedVariableGEOEASIndex()-1
                     );

    //reverse FFT the edited data (result is written back to the input data array).
    Util::fft3D( cg->getNX(), cg->getNY(), cg->getNZ(), data,
                 FFTComputationMode::REVERSE, FFTImageType::POLAR_FORM );

    //add the in-memory data (now in real space) to the new Cartesian grid object
    cgFFTtmp->addDataColumns( data, "real part of rFFT", "imaginary part of rFFT" );

    //save the grid to filesystem
    cgFFTtmp->writeToFS();

    //display the grid in real space (real part, GEO-EAS index == 1, first column in GEO-EAS file)
    Util::viewGrid( cgFFTtmp->getAttributeFromGEOEASIndex(1), this );
}

void ImageJockeyDialog::restore()
{
    //assuming the selected file is a Cartesian grid
    CartesianGrid* cg = (CartesianGrid*)m_cgSelector->getSelectedDataFile();
    if( ! cg )
        return;

    //delete data in memory (possibly edited)
    cg->freeLoadedData();

    //reread data from filesystem.
    cg->loadData();

    //update the 2D spectrogram plot
    spectrogramGridReplot();

    //causes an update in the 1D spectrogram widget
    //TODO: this is not very elegant
    m_spectrogram1Dparams->setAzimuth( m_spectrogram1Dparams->azimuth() );
}




void ImageJockeyDialog::onSVD()
{
    CartesianGrid* cg = (CartesianGrid*)m_cgSelector->getSelectedDataFile();
    if( ! cg ){
        QMessageBox::critical( this, "Error", QString("No Cartesian grid selected."));
        return;
    }

    bool ok;
    QString var_suffix = QInputDialog::getText(this, "User input requested.", "Suffixes for the SVD factors:", QLineEdit::Normal,
                                             "/SVD_", &ok);
    if( ! ok )
        return;

    long selectedAttributeIndex = m_atSelector->getSelectedVariableGEOEASIndex()-1;
    SVDParametersDialog dlg( this );
    int userResponse = dlg.exec();

    if( userResponse != QDialog::Accepted )
        return;

    spectral::array* a = cg->createSpectralArray( selectedAttributeIndex );
    spectral::SVD svd = spectral::svd( *a );

    long numberOfFactors = dlg.getNumberOfFactors();

    QString baseFactorName = m_atSelector->getSelectedVariableName();
    for (long i = 0; i < numberOfFactors; ++i) {
        spectral::array factor = svd.factor(i);
        QString factorName = baseFactorName + var_suffix + QString::number( i + 1 );
        cg->append( factorName, factor );
    }

    delete a;

    //    auto weights = svd.factor_weights();
    //    renderDecayingCurve(weights);
}
