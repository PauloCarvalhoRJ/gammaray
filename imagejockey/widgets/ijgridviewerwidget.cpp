#include "ijgridviewerwidget.h"
#include "ui_ijgridviewerwidget.h"
#include "spectral/svd.h"
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QProgressDialog>
#include "../imagejockeygridplot.h"
#include "../svd/svdfactor.h"
#include "../imagejockeyutils.h"

/*static*/ QString IJGridViewerWidget::m_lastOpenedPath = "";

IJGridViewerWidget::IJGridViewerWidget(bool deleteFactorOnClose, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::IJGridViewerWidget),
    m_factor( nullptr ),
    m_deleteFactorOnClose( deleteFactorOnClose )
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
    if( this->parentWidget() )
        ui->frmBottom->hide();
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

