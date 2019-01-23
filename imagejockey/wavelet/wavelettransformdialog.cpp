#include "wavelettransformdialog.h"
#include "ui_wavelettransformdialog.h"



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
    int waveletType = ui->cmbWaveletType->currentData().Int;
    bool interleaved = ( ui->cmbMethod->currentIndex() == 0 );
    WaveletUtils::transform( m_inputGrid,
                             m_inputVariableIndex,
                             getSelectedWaveletFamily(),
                             waveletType,
                             interleaved );
}

void WaveletTransformDialog::onWaveletFamilySelected( QString waveletFamilyName )
{
    //see which wavelet types are supported for each family in
    // http://www.gnu.org/software/gsl/manual/html_node/DWT-Initialization.html#DWT-Initialization
    ui->cmbWaveletType->clear();
    if( waveletFamilyName == "Daubechies" ){
        for( int i = 2; i < 21; i += 2)
            ui->cmbWaveletType->addItem( QString::number( i ), i );
        return;
    }
    if( waveletFamilyName == "Haar" ){
        ui->cmbWaveletType->addItem( "2", 2 );
        return;
    }
    if( waveletFamilyName == "B-Spline" ){
        //parameter is computed as i*100 + j
        ui->cmbWaveletType->addItem( "i=1, j=3", 103 );
        ui->cmbWaveletType->addItem( "i=1, j=5", 105 );
        ui->cmbWaveletType->addItem( "i=2, j=2", 202 );
        ui->cmbWaveletType->addItem( "i=2, j=4", 204 );
        ui->cmbWaveletType->addItem( "i=2, j=6", 206 );
        ui->cmbWaveletType->addItem( "i=2, j=8", 208 );
        ui->cmbWaveletType->addItem( "i=3, j=1", 301 );
        ui->cmbWaveletType->addItem( "i=3, j=3", 303 );
        ui->cmbWaveletType->addItem( "i=3, j=5", 305 );
        ui->cmbWaveletType->addItem( "i=3, j=7", 307 );
        ui->cmbWaveletType->addItem( "i=3, j=9", 309 );
        return;
    }
    QMessageBox::critical( this, "Error", QString("WaveletTransformDialog::onWaveletFamilySelected(): Unknown wavelet family: " + waveletFamilyName));
}

