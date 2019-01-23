#include "wavelettransformdialog.h"
#include "ui_wavelettransformdialog.h"

#include "spectral/spectral.h"
#include "imagejockey/svd/svdfactor.h"
#include "imagejockey/widgets/ijgridviewerwidget.h"

#include <QMessageBox>

WaveletTransformDialog::WaveletTransformDialog(IJAbstractCartesianGrid *inputGrid, uint inputVariableIndex, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WaveletTransformDialog),
    m_inputGrid( inputGrid ),
    m_inputVariableIndex( inputVariableIndex )
{
    ui->setupUi(this);
    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle( "Wavelet Transform Dialog" );

    onWaveletFamilySelected( ui->cmbWaveletFamily->currentText() );
}

WaveletTransformDialog::~WaveletTransformDialog()
{
    delete ui;
}

WaveletFamily WaveletTransformDialog::getSelectedWaveletFamily()
{
    QString waveletFamilyName = ui->cmbWaveletFamily->currentText();
    WaveletFamily waveletFamily = WaveletFamily::UNKNOWN;
    if( waveletFamilyName == "Daubechies" ){
        waveletFamily = WaveletFamily::DAUBECHIES;
    } else if( waveletFamilyName == "Haar" ){
        waveletFamily = WaveletFamily::HAAR;
    } else if( waveletFamilyName == "B-Spline" ){
        waveletFamily = WaveletFamily::B_SPLINE;
    } else {
        QMessageBox::critical( this, "Error", QString("WaveletTransformDialog::getSelectedWaveletFamily(): Unknown wavelet family: " + waveletFamilyName));
    }
    return waveletFamily;
}

void WaveletTransformDialog::onPerformTransform()
{
    int waveletType = ui->cmbWaveletType->itemData( ui->cmbWaveletType->currentIndex() ).toInt();
    bool interleaved = ( ui->cmbMethod->currentIndex() == 0 );
    m_DWTbuffer = WaveletUtils::transform( m_inputGrid,
                                           m_inputVariableIndex,
                                           getSelectedWaveletFamily(),
                                           waveletType,
                                           interleaved );
    debugGrid( m_DWTbuffer );
}

void WaveletTransformDialog::onWaveletFamilySelected( QString waveletFamilyName )
{
    //see which wavelet types are supported for each family in
    // http://www.gnu.org/software/gsl/manual/html_node/DWT-Initialization.html#DWT-Initialization
    ui->cmbWaveletType->clear();
    if( waveletFamilyName == "Daubechies" ){
        for( int i = 4; i < 21; i += 2)
            ui->cmbWaveletType->addItem( QString::number( i ), QVariant( i ) );
        return;
    }
    if( waveletFamilyName == "Haar" ){
        ui->cmbWaveletType->addItem( "2", QVariant( 2 ) );
        return;
    }
    if( waveletFamilyName == "B-Spline" ){
        //parameter is computed as i*100 + j
        ui->cmbWaveletType->addItem( "i=1, j=3", QVariant( 103 ));
        ui->cmbWaveletType->addItem( "i=1, j=5", QVariant( 105 ));
        ui->cmbWaveletType->addItem( "i=2, j=2", QVariant( 202 ));
        ui->cmbWaveletType->addItem( "i=2, j=4", QVariant( 204 ));
        ui->cmbWaveletType->addItem( "i=2, j=6", QVariant( 206 ));
        ui->cmbWaveletType->addItem( "i=2, j=8", QVariant( 208 ));
        ui->cmbWaveletType->addItem( "i=3, j=1", QVariant( 301 ));
        ui->cmbWaveletType->addItem( "i=3, j=3", QVariant( 303 ));
        ui->cmbWaveletType->addItem( "i=3, j=5", QVariant( 305 ));
        ui->cmbWaveletType->addItem( "i=3, j=7", QVariant( 307 ));
        ui->cmbWaveletType->addItem( "i=3, j=9", QVariant( 309 ));
        return;
    }
    QMessageBox::critical( this, "Error", QString("WaveletTransformDialog::onWaveletFamilySelected(): Unknown wavelet family: " + waveletFamilyName));
}

void WaveletTransformDialog::onSaveDWTResultAsGrid()
{
    emit saveDWTTransform( ui->txtCoeffVariableName->text(), m_DWTbuffer );
}

void WaveletTransformDialog::debugGrid(const spectral::array &grid)
{
    spectral::array result ( grid );
    SVDFactor* gridSVD = new SVDFactor( std::move(result), 1, 0.42,
                                     0.0,
                                     0.0,
                                     0.0,
                                     1.0,
                                     1.0,
                                     1.0, 0.0 );
    IJGridViewerWidget* ijgv = new IJGridViewerWidget( true, false, true );
    ijgv->setFactor( gridSVD );
    ijgv->show();
}
