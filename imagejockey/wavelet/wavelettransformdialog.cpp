#include "wavelettransformdialog.h"
#include "ui_wavelettransformdialog.h"

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

void WaveletTransformDialog::onPerformTransform()
{

}
