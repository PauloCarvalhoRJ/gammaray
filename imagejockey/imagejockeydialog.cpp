#include "imagejockeydialog.h"
#include "ui_imagejockeydialog.h"
#include "widgets/ijcartesiangridselector.h"
#include "widgets/ijvariableselector.h"
#include "widgets/grcompass.h"
#include "ijabstractcartesiangrid.h"
#include "ijabstractvariable.h"
#include "imagejockeygridplot.h"
#include "spectrogram1dparameters.h"
#include "spectrogram1dplot.h"
#include "equalizer/equalizerwidget.h"
#include "imagejockeyutils.h"
#include "svd/svdparametersdialog.h"
#include "svd/svdfactor.h"
#include "svd/svdfactortree.h"
#include "svd/svdanalysisdialog.h"
#include "svd/svdfactorsel/svdfactorsselectiondialog.h"
#include "widgets/ijgridviewerwidget.h"

#include "spectral/svd.h" //third-party Eigen

#include <QInputDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QThread>
#include <qwt_wheel.h>

ImageJockeyDialog::ImageJockeyDialog(const std::vector<IJAbstractCartesianGrid *> &grids, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImageJockeyDialog),
    m_spectrogram1Dparams( new Spectrogram1DParameters() ),
    m_numberOfSVDFactorsSetInTheDialog( 0 ),
    m_grids( grids )
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
    m_cgSelector = new IJCartesianGridSelector( m_grids, true );
    ui->frmCmbGridPlaceholder->layout()->addWidget( m_cgSelector );

    //the combo box to choose the variable with the real part
    m_varAmplitudeSelector = new IJVariableSelector();
    ui->frmCmbAttributePlaceholder->layout()->addWidget( m_varAmplitudeSelector );
    connect( m_cgSelector, SIGNAL(cartesianGridSelected(IJAbstractCartesianGrid*)),
             m_varAmplitudeSelector, SLOT(onListVariables(IJAbstractCartesianGrid*)) );
    connect( m_varAmplitudeSelector, SIGNAL(variableSelected(IJAbstractVariable*)),
             this, SLOT(onUpdateGridPlot(IJAbstractVariable*)));
    connect( m_varAmplitudeSelector, SIGNAL(errorOccurred(QString)), this, SIGNAL(errorOccurred(QString)) );
    connect( m_varAmplitudeSelector, SIGNAL(warningOccurred(QString)), this, SIGNAL(warningOccurred(QString)) );

    //the combo box to choose the variable with the imaginary part
    m_varPhaseSelector = new IJVariableSelector();
    ui->frmCmbImagPartPlaceholder->layout()->addWidget( m_varPhaseSelector );
    connect( m_cgSelector, SIGNAL(cartesianGridSelected(IJAbstractCartesianGrid*)),
             m_varPhaseSelector, SLOT(onListVariables(IJAbstractCartesianGrid*)) );
    connect( m_varPhaseSelector, SIGNAL(errorOccurred(QString)), this, SIGNAL(errorOccurred(QString)) );
    connect( m_varPhaseSelector, SIGNAL(warningOccurred(QString)), this, SIGNAL(warningOccurred(QString)) );

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

	//Capture any errors that may occur in the widgets.
	connect( m_spectrogram1Dparams, SIGNAL(errorOccurred(QString)), this, SLOT(onWidgetErrorOccurred(QString)));
	connect( m_gridPlot, SIGNAL(errorOccurred(QString)), this, SLOT(onWidgetErrorOccurred(QString)));
	connect( m_spectrogram1Dplot, SIGNAL(errorOccurred(QString)), this, SLOT( onWidgetErrorOccurred(QString)));

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
    emit infoOccurred("ImageJockeyDialog destroyed.");
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

void ImageJockeyDialog::onUpdateGridPlot(IJAbstractVariable *var)
{
    //set the attribute
    m_gridPlot->setVariable( var );

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
    int variableIndex = var->getIndexInParentGrid();
    IJAbstractCartesianGrid* cg = var->getParentGrid();
    cg->dataWillBeRequested();
    double dataAbsMin = cg->absMin( variableIndex );
    double dataAbsMax = cg->absMax( variableIndex );
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
	m_spectrogram1Dparams->setRefCenter( cg->getCenterLocation() );

    //set the attribute for the 1D spectrogram plot
    m_spectrogram1Dplot->setVariable( var );

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
    IJAbstractCartesianGrid* cg = m_cgSelector->getSelectedGrid();
    if( ! cg )
        return;

    //get the variable
    IJAbstractVariable* var = cg->getVariableByIndex( m_varAmplitudeSelector->getSelectedVariableIndex() );

    //get the geometry of the area of influence in the 2D spectrogram.
    //The central frequency is the distance from the center of the 2D spectrogram.
    //The resulting area is defined taking into account the azimuth, which is set
    //   in the Spectrogram1DParamaters object.
    QList<QPointF> aoi = m_spectrogram1Dparams->getAreaOfInfluence( centralFrequency, m_equalizerWidget->getFrequencyStep() );

    //get the half-band geometry, as the azimuth tolerance setting may cause the final selection
    //to clip the area-of-influence
    QList<QPointF> halfBand = m_spectrogram1Dparams->getHalfBandGeometry();

    //perform the equalization of values
    cg->equalizeValues( aoi, delta_dB, var->getIndexInParentGrid(), m_wheelColorDecibelReference->value(), halfBand );

    //mirror the area of influence about the center of the 2D spectrogram
    ImageJockeyUtils::mirror2D( aoi, cg->getCenterLocation() );

    //mirror the half-band geometry about the center of the 2D spectrogram
    ImageJockeyUtils::mirror2D( halfBand, cg->getCenterLocation() );

    //perform the equalization in the opposite area to preserve the 2D spectrogram's symmetry
    cg->equalizeValues( aoi, delta_dB, var->getIndexInParentGrid(), m_wheelColorDecibelReference->value(), halfBand );

    //update the 2D spectrogram plot
    spectrogramGridReplot();

    //causes an update in the 1D spectrogram widget
    //TODO: this is not very elegant
    m_spectrogram1Dparams->setAzimuth( m_spectrogram1Dparams->azimuth() );
}

void ImageJockeyDialog::save()
{
    //get the Cartesian grid
    IJAbstractCartesianGrid* cg = m_cgSelector->getSelectedGrid();
    if( ! cg )
        return;

    //save edited Fourier image to filesystem
    cg->saveData();

    emit infoOccurred("ImageJockeyDialog::save(): file saved.");
}

void ImageJockeyDialog::preview()
{
    //TODO: the working version of this function was copied to MainWindow (onPreviewRFFTImageJockey() slot)
    //      The working version uses VTK's RFFT and GammaRay infrastructure.

    //Get the Cartesian grid with the Fourier image in polar form.
    IJAbstractCartesianGrid* cg = m_cgSelector->getSelectedGrid();
    if( ! cg )
        return;

    //create the array with the input de-shifted and in rectangular form.
    spectral::complex_array dataReady( (spectral::index)cg->getNI(),
                                       (spectral::index)cg->getNJ(),
                                       (spectral::index)cg->getNK() );

    //De-shift frequencies, convert the complex numbers to rectangular form ( a + bi ) and
    //change the scan order from GSLib convention to FFTW3 convention.
    if( ! ImageJockeyUtils::prepareToFFTW3reverseFFT( cg,
                                                m_varAmplitudeSelector->getSelectedVariableIndex(),
                                                cg,
                                                m_varPhaseSelector->getSelectedVariableIndex(),
                                                dataReady ) ){
        return;
    }

    //Create the output array.
    spectral::array outputData( (spectral::index)cg->getNI(),
                                (spectral::index)cg->getNJ(),
                                (spectral::index)cg->getNK() );

    //Apply reverse FFT.
    {
        QProgressDialog progressDialog;
        progressDialog.setRange(0,0);
        progressDialog.show();
        progressDialog.setLabelText("Computing RFFT...");
        QCoreApplication::processEvents(); //let Qt repaint widgets
        spectral::backward( outputData, dataReady );
    }

    //Construct a displayable object from the result.
    SVDFactor* factor = new SVDFactor( std::move(outputData), 1, 1, cg->getOriginX(), cg->getOriginY(), cg->getOriginZ(),
                                       cg->getCellSizeI(), cg->getCellSizeJ(), cg->getCellSizeK(), SVDFactor::getSVDFactorTreeSplitThreshold());

    //Opens the viewer.
    IJGridViewerWidget* ijgvw = new IJGridViewerWidget( true );
    factor->setCustomName("Reverse FFT");
    ijgvw->setFactor( factor );
    ijgvw->show();
}

void ImageJockeyDialog::restore()
{
    //get the Cartesian grid
    IJAbstractCartesianGrid* cg = m_cgSelector->getSelectedGrid();
    if( ! cg )
        return;

    //delete data in memory (possibly edited)
    cg->clearLoadedData();

    //reread data from filesystem.
    cg->dataWillBeRequested();

    //update the 2D spectrogram plot
    spectrogramGridReplot();

    //causes an update in the 1D spectrogram widget
    //TODO: this is not very elegant
    m_spectrogram1Dparams->setAzimuth( m_spectrogram1Dparams->azimuth() );
}

void ImageJockeyDialog::onSVD()
{
    //Get the selected Cartesian grid containing a Fourier image or experimental variogram (e.g. for Factorial kriging)
    IJAbstractCartesianGrid* cg = m_cgSelector->getSelectedGrid();
    if( ! cg ){
        QMessageBox::critical( this, "Error", QString("No Cartesian grid selected."));
        return;
    }

	//Get the data
    long selectedAttributeIndex = m_varAmplitudeSelector->getSelectedVariableIndex();
    spectral::array* a = cg->createSpectralArray( selectedAttributeIndex );

	//Get the grid geometry parameters (useful for displaying)
    double x0 = cg->getOriginX();
    double y0 = cg->getOriginY();
    double z0 = cg->getOriginZ();
    double dx = cg->getCellSizeI();
    double dy = cg->getCellSizeJ();
    double dz = cg->getCellSizeK();

	//Compute SVD
	QProgressDialog progressDialog;
	progressDialog.setRange(0,0);
	progressDialog.setLabelText("Computing SVD factors...");
	progressDialog.show();
	QCoreApplication::processEvents();
	spectral::SVD svd = spectral::svd( *a );
	progressDialog.hide();

    //get the list with the factor weights (information quantity)
    spectral::array weights = svd.factor_weights();
    emit infoOccurred("ImageJockeyDialog::onSVD(): " + QString::number( weights.data().size() ) + " factor(s) were found.");

    //User enters number of SVD factors
    SVDFactorsSelectionDialog * svdfsd = new SVDFactorsSelectionDialog( weights.data(), true, this );
    connect( svdfsd, SIGNAL(numberOfFactorsSelected(int)), this, SLOT(onUserSetNumberOfSVDFactors(int)) );
    int userResponse = svdfsd->exec();
    if( userResponse != QDialog::Accepted )
        return;
    long numberOfFactors = m_numberOfSVDFactorsSetInTheDialog;

    //Create the structure to store the SVD factors
    SVDFactorTree * factorTree = new SVDFactorTree( SVDFactor::getSVDFactorTreeSplitThreshold( true ) );

	//Get the desired SVD factors
    {
		QProgressDialog progressDialog;
		progressDialog.setRange(0,0);
		progressDialog.show();
        for (long i = 0; i < numberOfFactors; ++i) {
			progressDialog.setLabelText("Retrieving SVD factor " + QString::number(i+1) + " of " + QString::number(numberOfFactors) + "...");
			QCoreApplication::processEvents();
			spectral::array factor = svd.factor(i);
            SVDFactor* svdFactor = new SVDFactor( std::move(factor), i + 1, weights[i], x0, y0, z0, dx, dy, dz, SVDFactor::getSVDFactorTreeSplitThreshold() );
			factorTree->addFirstLevelFactor( svdFactor );
            //cg->append( factorName, factor );
        }
    }

    //delete the data array, since it's not necessary anymore
    delete a;

    if( numberOfFactors > 0 ){
        //show the SDV analysis dialog
        SVDAnalysisDialog* svdad = new SVDAnalysisDialog( this );
        connect( svdad, SIGNAL(sumOfFactorsComputed(spectral::array*)),
                 this, SLOT(onSumOfFactorsWasComputed(spectral::array*)) );
        svdad->setTree( factorTree );
        //setting these enables RFFT preview in the SVD Analysis dialog
        svdad->setGridWithPhaseForPossibleRFFT( cg, m_varPhaseSelector->getSelectedVariableIndex() );
        svdad->setDeleteTreeOnClose( true );
        svdad->show();
    } else {
        emit errorOccurred("ImageJockeyDialog::onSVD(): user set zero factors. Aborted.");
        delete factorTree;
    }
}

void ImageJockeyDialog::onUserSetNumberOfSVDFactors(int number)
{
    m_numberOfSVDFactorsSetInTheDialog = number;
}

void ImageJockeyDialog::onSumOfFactorsWasComputed(spectral::array *sumOfFactors)
{
    //propose a name for the new variable in the source grid
    QString proposed_name( m_varAmplitudeSelector->getSelectedVariableName() );
    proposed_name.append( "_SVD" );

    //open the renaming dialog
    bool ok;
    QString new_variable_name = QInputDialog::getText(this, "Name the new variable",
                                             "New variable from individual or sum of SVD factors:", QLineEdit::Normal,
                                             proposed_name, &ok);
    if( ! ok ){
        delete sumOfFactors; //discard the computed sum
        return;
    }

    //save the sum to the grid in the project
    IJAbstractCartesianGrid* cg = m_cgSelector->getSelectedGrid();
    cg->appendAsNewVariable(new_variable_name, *sumOfFactors );

    //discard the computed sum
	delete sumOfFactors;
}

void ImageJockeyDialog::onWidgetErrorOccurred(QString message)
{
    emit errorOccurred( message );
}

void ImageJockeyDialog::onWidgetWarningOccurred(QString message)
{
    emit warningOccurred( message );
}
