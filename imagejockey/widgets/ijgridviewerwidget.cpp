#include "ijgridviewerwidget.h"
#include "ui_ijgridviewerwidget.h"
#include "spectral/svd.h"
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include "../imagejockeygridplot.h"
#include "../svd/svdfactor.h"
#include "../imagejockeyutils.h"

/*static*/ QString IJGridViewerWidget::m_lastOpenedPath = "";

IJGridViewerWidget::IJGridViewerWidget(bool deleteFactorOnClose, bool showSaveButton, bool showDismissButton, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IJGridViewerWidget),
    m_factor( nullptr ),
	m_deleteFactorOnClose( deleteFactorOnClose ),
	m_dataChanged( false ),
	m_showSaveButton( showSaveButton ),
	m_showDismissButton( showDismissButton )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
	this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle( "Quick grid viewer" );

	//add a grid plot widget to the right pane of the dialog
	m_gridPlot = new ImageJockeyGridPlot();
	ui->frmRightTop->layout()->addWidget( m_gridPlot );

    connect( ui->dblSpinColorScaleMin, SIGNAL(valueChanged(double)), m_gridPlot, SLOT(setColorScaleMin(double)));
    connect( ui->dblSpinColorScaleMax, SIGNAL(valueChanged(double)), m_gridPlot, SLOT(setColorScaleMax(double)));
    connect( ui->cmbColorScale, SIGNAL(currentIndexChanged(int)), this, SLOT(onCmbColorScaleValueChanged(int)));

    connect( ui->cmbPlane, SIGNAL(currentIndexChanged(int)), this, SLOT(onCmbPlaneChanged(int)));
    connect( ui->spinSlice, SIGNAL(valueChanged(int)), this, SLOT(onSpinSliceChanged(int)));

    //Hide the dismiss button if this widget has a parent (not stand alone).
	if( ! m_showDismissButton )
		ui->btnDismiss->hide();

	//Hide the "Save" button accordingly to the respective flag.
	if( ! m_showSaveButton )
		ui->btnSave->hide();
}

IJGridViewerWidget::~IJGridViewerWidget()
{
    if( m_deleteFactorOnClose && m_factor )
        delete m_factor;
    delete ui;
}


void IJGridViewerWidget::forcePlotUpdate()
{
    m_gridPlot->forceUpdate();
}

void IJGridViewerWidget::adjustColorTableWidgets(int cmbIndex)
{
    if( ! m_factor )
        return;
    //adjust the color table widgets
    double max = m_factor->getMaxValue();
    double min = m_factor->getMinValue();
    double valueMin = ui->dblSpinColorScaleMin->value();
    double valueMax = ui->dblSpinColorScaleMax->value();
    if( cmbIndex == 0 ) {//currently log, switching to linear
        if( std::isnan( valueMax ) )
            valueMax = max;
        else
            valueMax = std::pow( 10, valueMax );
        if( std::isnan( valueMin ) )
            valueMin = min;
        else
            valueMin = std::pow( 10, valueMin );
    }
    if( cmbIndex == 1 ){ //currently linear, switching to log
        max = std::log10( max );
        if( min < 0)
            min = 1e-6;
        min = std::log10( min );
        valueMax = std::log10( valueMax );
        if( valueMin < 0)
            valueMin = 1e-6;
        valueMin = std::log10( valueMin );
    }
    ui->dblSpinColorScaleMax->setMaximum( max );
    ui->dblSpinColorScaleMin->setMaximum( max );
    ui->dblSpinColorScaleMax->setMinimum( min );
    ui->dblSpinColorScaleMin->setMinimum( min );
    ui->dblSpinColorScaleMax->setValue( valueMax );
    ui->dblSpinColorScaleMin->setValue( valueMin );

}


void IJGridViewerWidget::setFactor(SVDFactor *factor)
{
    m_factor = factor;
    if( ! m_factor )
        return;
    m_gridPlot->setSVDFactor( m_factor );
    double min = m_factor->getMinValue();
    double max = m_factor->getMaxValue();
	m_gridPlot->setColorScaleMin( min );
	m_gridPlot->setColorScaleMax( max );
	ui->dblSpinColorScaleMax->setMaximum( max );
	ui->dblSpinColorScaleMax->setMinimum( min );
	ui->dblSpinColorScaleMax->setValue( max );
	ui->dblSpinColorScaleMin->setMaximum( max );
	ui->dblSpinColorScaleMin->setMinimum( min );
	ui->dblSpinColorScaleMin->setValue( min );
    ui->spinSlice->setMaximum( m_factor->getCurrentPlaneNumberOfSlices()-1 ); //1st == 0; last == total-1
    if( ui->cmbColorScale->currentIndex() == 1 )
		adjustColorTableWidgets( 1 );
}

void IJGridViewerWidget::onCmbColorScaleValueChanged(int index)
{
    //change color scaling
    if( index == 0 )
        m_gridPlot->setColorScaleForSVDFactor( ColorScaleForSVDFactor::LINEAR );
    if( index == 1 )
        m_gridPlot->setColorScaleForSVDFactor( ColorScaleForSVDFactor::LOG );

    //adjust the color table max/min spin widgets
    adjustColorTableWidgets( index );

    forcePlotUpdate();
}

void IJGridViewerWidget::onCmbPlaneChanged(int index)
{
    if( ! m_factor )
        return;

    if( index == 0 )
        m_factor->setPlaneOrientation( SVDFactorPlaneOrientation::XY );
    if( index == 1 )
        m_factor->setPlaneOrientation( SVDFactorPlaneOrientation::XZ );
    if( index == 2 )
        m_factor->setPlaneOrientation( SVDFactorPlaneOrientation::YZ );

    ui->spinSlice->setMaximum( m_factor->getCurrentPlaneNumberOfSlices()-1 ); //1st == 0; last == total-1

    setFactor( m_factor );

    forcePlotUpdate();
}

void IJGridViewerWidget::onSpinSliceChanged(int value)
{
    if( ! m_factor )
        return;

    m_factor->setCurrentSlice( value );

    forcePlotUpdate();
}

void IJGridViewerWidget::onDismiss()
{
	emit closed( m_factor, m_dataChanged );
	this->close();
}

void IJGridViewerWidget::onExportSliceAsPNG()
{
    //User enters file path to save the image.
    QString filePath = QFileDialog::getSaveFileName( this, "Select directory for image",
                                                     IJGridViewerWidget::m_lastOpenedPath,
                                                     "*.png");
    if( ! filePath.isEmpty() ){
        //Get the entered directory for later reuse.
        QFileInfo fileInfo( filePath );
        IJGridViewerWidget::m_lastOpenedPath = fileInfo.dir().absolutePath();
    }else
        return;

    //Get the 2D grid of the currently viewed grid slice.
    SVDFactor* slice =  m_factor->createFactorFromCurrent2DSlice();

    //Get the value extrema for rescaling to 0-255 gray levels.
    double max = m_factor->getMaxValue();
    double min = m_factor->getMinValue();

    //Create an image object equivalent to the grid slice.
    QImage image( slice->getNI(), slice->getNJ(), QImage::Format_ARGB32 );

    //Set the pixel gray levels ( image J origin is at top left )
    for( int j = 0; j < slice->getNJ(); ++j )
        for( int i = 0; i < slice->getNI(); ++i ){
            double value = slice->dataIJK( i, j, 0 );
            uint level = 255 * (value - min) / (max - min);
            uint alpha = ( slice->isNDV( value ) ? 0 : 255 );
            image.setPixelColor( QPoint(i,slice->getNJ()-1-j), QColor( level, level, level, alpha ) );
        }

    //Save pixel data as PNG image.
    QPixmap pixmap;
    pixmap.convertFromImage( image );
    QFile file( filePath );
    file.open(QIODevice::WriteOnly);
    pixmap.save(&file, "PNG");

    //Opens the image.
    QDesktopServices::openUrl(QUrl::fromLocalFile( filePath ));

    //Discard the slice data.
    delete slice;
}

void IJGridViewerWidget::onImportSliceDataFromPNG()
{
    //User enters file path to choose the image to load.
    QString filePath = QFileDialog::getOpenFileName( this, "Select image file",
                                                     IJGridViewerWidget::m_lastOpenedPath,
                                                     "*.png");
    if( ! filePath.isEmpty() ){
        //Get the entered directory for later reuse.
        QFileInfo fileInfo( filePath );
        IJGridViewerWidget::m_lastOpenedPath = fileInfo.dir().absolutePath();
    }else
        return;

    //Load PNG file.
    QPixmap pixmap;
    pixmap.load( filePath, "PNG" );

    //Get the 2D grid of the currently viewed grid slice.
    SVDFactor* slice =  m_factor->createFactorFromCurrent2DSlice();

    //Check weather the image as the slice are compatible.
    if( pixmap.width() != slice->getNI() || pixmap.height() != slice->getNJ() ){
        QMessageBox::critical( this, "Error", QString("Image and current slice are incompatible."));
        delete slice;
        return;
    }

    //Get the value extrema for rescaling from 0-255 gray levels.
    double max = m_factor->getMaxValue();
    double min = m_factor->getMinValue();

    //If the grid is constant valued, it is not possible to get the scale.
    //Then, scale max will be forced to be min + 1.0.
    bool scaleWasForciblyChanged = false;
    if( ImageJockeyUtils::almostEqual2sComplement( min, max, 1) ){
        QMessageBox::warning( this, "Warning",
          QString("Current grid is constant valued.  Imported values will be re-scaled from min to min + 1.0."));
        max = min + 1.0;
        scaleWasForciblyChanged = true;
    }

    //Get the QImage object, so we can access individual pixel data.
    QImage image = pixmap.toImage();

    //Get the pixel gray levels ( image J origin is at top left )
    for( int j = 0; j < slice->getNJ(); ++j )
        for( int i = 0; i < slice->getNI(); ++i ){
            //Get the color of the pixel.
			QColor color = image.pixelColor( i, slice->getNJ()-1-j );
            //Check whether it is gray.
            if( color.red() != color.blue() || color.red() != color.green() ){
                QMessageBox::critical( this, "Error", QString("Image contains non-gray pixels."));
                delete slice;
                return;
            }
            //Compute the value from the gray level
            int level = color.blue(); //could be either from red or green.
            double value = min + level / 255.0 * (max - min);
            //If the pixel is transparent, set the value to NaN.
            if( color.alpha() == 0 )
                value = std::numeric_limits<double>::quiet_NaN();
            //Set the data value.
            slice->setDataIJK( i, j, 0, value );
        }

	//Set imported slice data.
	m_factor->setSlice( slice );

    //Discard the slice data.
    delete slice;

    //If the scale was artificially changed, re-set the factor
    //so the widgets change accordingly.
    if( scaleWasForciblyChanged )
        setFactor( m_factor );

	//Update the plot.
	forcePlotUpdate();

	//Set that the data was changed.
	m_dataChanged = true;
}

void IJGridViewerWidget::onSave()
{
	emit save( m_factor );
}
