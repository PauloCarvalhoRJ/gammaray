#include "emdanalysisdialog.h"
#include "ui_emdanalysisdialog.h"
#include "spectral/spectral.h"

EMDAnalysisDialog::EMDAnalysisDialog(IJAbstractCartesianGridPtr inputGrid, uint inputVariableIndex, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EMDAnalysisDialog),
    m_inputGrid( inputGrid ),
    m_inputVariableIndex( inputVariableIndex )
{
    ui->setupUi(this);

    setWindowTitle( "Empirical Mode Decomposition" );

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);
}

EMDAnalysisDialog::~EMDAnalysisDialog()
{
    delete ui;
}

void EMDAnalysisDialog::onPerformEMD()
{
    //fetch data from file
    m_inputGrid->dataWillBeRequested();

    //get grid dimensions
    int nI = m_inputGrid->getNI();
    int nJ = m_inputGrid->getNJ();
    int nK = m_inputGrid->getNK();

    //get the null data value
    double NDV = m_inputGrid->getUninformedDataValue();

    //create the local maxima envelope data set, initialized with no-data-values.
    spectral::array localMaximaEnvelope( nI, nJ, nK, NDV );

    //create the local minima envelope data set, initialized with no-data-values.
    spectral::array localMinimaEnvelope( nI, nJ, nK, NDV );

    //size of half-window (1 means a 3x3x3 window)
    int halfWindowSize = 1;

    //for each cell...
    for( int k = 0; k < nK; ++k )
        for( int j = 0; j < nJ; ++j )
            for( int i = 0; i < nI; ++i ){
                //...get its value
                double cellValue = m_inputGrid->getData( m_inputVariableIndex, i, j, k );
                //...get the neighboring values
                for( int offsetK = -halfWindowSize; offsetK <= halfWindowSize; ++offsetK )
                    for( int offsetJ = -halfWindowSize; offsetJ <= halfWindowSize; ++offsetJ )
                        for( int offsetI = -halfWindowSize; offsetI <= halfWindowSize; ++offsetI ){
                            int neighI = nI + offsetI;
                            int neighJ = nJ + offsetJ;
                            int neighK = nK + offsetK;
                        }
            }


}
