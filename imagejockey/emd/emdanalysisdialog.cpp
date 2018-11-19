#include "emdanalysisdialog.h"
#include "ui_emdanalysisdialog.h"
#include "spectral/spectral.h"
#include "imagejockey/widgets/ijgridviewerwidget.h"
#include "imagejockey/svd/svdfactor.h"
#include "imagejockey/widgets/ijquick3dviewer.h"
#include <vtkSmartPointer.h>
#include <vtkPoints.h>
#include <vtkDoubleArray.h>
#include <vtkShepardMethod.h>
#include <vtkImageData.h>

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

    //get the null data value (NaN for a spectral::array single-variable grid)
    double NDV = std::numeric_limits<double>::quiet_NaN();

    //create the local maxima envelope data set, initialized with no-data-values.
    spectral::array localMaximaEnvelope( nI, nJ, nK, NDV );

    //create the local minima envelope data set, initialized with no-data-values.
    spectral::array localMinimaEnvelope( nI, nJ, nK, NDV );

    //count the local extrema count
    uint localMaximaCount = 0;
    uint localMinimaCount = 0;

    //size of half-window (1 means a 3x3x3 window with the target cell in the center)
    int halfWindowSize = 1;

    //-------------- The loop to find the local extrema for the envelopes -----------------
    //for each cell...
    for( int k = 0; k < nK; ++k )
        for( int j = 0; j < nJ; ++j )
            for( int i = 0; i < nI; ++i ){
                bool is_a_local_minimum = true;
                bool is_a_local_maximum = true;
                //...get its value
                double cellValue = m_inputGrid->getData( m_inputVariableIndex, i, j, k );
                //...evaluate the neighboring values
                for( int offsetK = -halfWindowSize; offsetK <= halfWindowSize; ++offsetK )
                    for( int offsetJ = -halfWindowSize; offsetJ <= halfWindowSize; ++offsetJ )
                        for( int offsetI = -halfWindowSize; offsetI <= halfWindowSize; ++offsetI ){
                            int neighI = i + offsetI;
                            int neighJ = j + offsetJ;
                            int neighK = k + offsetK;
                            //if's to handle border cases
                            if( neighI >= 0 && neighI < nI &&
                                neighJ >= 0 && neighJ < nJ &&
                                neighK >= 0 && neighK < nK &&
                                ! (neighI == i && neighJ == j && neighK == k) /*do not compare with itself*/ ){
                                //get neighboring value
                                double neighValue = m_inputGrid->getData( m_inputVariableIndex,
                                                                          neighI,
                                                                          neighJ,
                                                                          neighK );
                                //if it is a valid value
                                if( ! m_inputGrid->isNoDataValue( neighValue )){
                                    //if the cell value is less than a neighbor's...
                                    if( cellValue < neighValue )
                                        //...it can't be a local maximum
                                        is_a_local_maximum = false;
                                    //if the cell value is greater than a neighbor's...
                                    if( cellValue > neighValue )
                                        //...it can't be a local minimum
                                        is_a_local_minimum = false;
                                }
                            }
                        } // --- evaluate the neighboring cells
                //if the cell is a local maximum...
                if( is_a_local_maximum ){
                    //... assign the value to the grid of the local maxima envelope
                    localMaximaEnvelope( i, j, k ) = cellValue;
                    ++localMaximaCount;
                }
                //if the cell is a local minimum...
                if( is_a_local_minimum ){
                    //... assign the value to the grid of the local minima envelope
                    localMinimaEnvelope( i, j, k ) = cellValue;
                    ++localMinimaCount;
                }
            } // --- for each cell
    //----------------------------------------------------------------------------------------

//    IJGridViewerWidget* ijgw = new IJGridViewerWidget( true, false, true, this );
//    SVDFactor* grid = new SVDFactor( std::move(localMinimaEnvelope), 1, 1.0, 0.0, 0.0, 0.0,
//                                     m_inputGrid->getCellSizeI(),
//                                     m_inputGrid->getCellSizeJ(),
//                                     m_inputGrid->getCellSizeK(),
//                                     0.42
//                                     );
//    ijgw->setFactor( grid );
//    ijgw->show();


    //--------------------------interpolate the local extrema--------------------------------

    //the VTK collections of points in space of the local extrema
    vtkSmartPointer<vtkPoints> pointsMaxima = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkPoints> pointsMinima = vtkSmartPointer<vtkPoints>::New();

    //the VTK collections of values of local extrema
    vtkSmartPointer<vtkDoubleArray> valuesMaxima = vtkSmartPointer<vtkDoubleArray>::New();
    vtkSmartPointer<vtkDoubleArray> valuesMinima = vtkSmartPointer<vtkDoubleArray>::New();
    valuesMaxima->SetNumberOfComponents(1);
    valuesMaxima->SetName("ValuesMaxima");
    valuesMinima->SetNumberOfComponents(1);
    valuesMinima->SetName("ValuesMinima");

    //populate the VTK collections above
    double x, y, z;
    for( int k = 0; k < nK; ++k )
        for( int j = 0; j < nJ; ++j )
            for( int i = 0; i < nI; ++i ){
                double maximum = localMaximaEnvelope( i, j, k );
                if( std::isfinite( maximum ) ){
                    m_inputGrid->getCellLocation( i, j, k, x, y, z );
                    pointsMaxima->InsertNextPoint( x, y, z );
                    valuesMaxima->InsertNextValue( maximum );
                }
                double minimum = localMinimaEnvelope( i, j, k );
                if( std::isfinite( minimum ) ){
                    m_inputGrid->getCellLocation( i, j, k, x, y, z );
                    pointsMinima->InsertNextPoint( x, y, z );
                    valuesMinima->InsertNextValue( minimum );
                }
            }

    //prepare VTK polygonal objects for interpolation
    vtkSmartPointer<vtkPoints> polydataToProcessMaxima = vtkSmartPointer<vtkPoints>::New();
    polydataToProcessMaxima->SetPoints( pointsMaxima );
    polydataToProcessMaxima->GetPointData()->SetScalars( valuesMaxima );
    vtkSmartPointer<vtkPoints> polydataToProcessMinima = vtkSmartPointer<vtkPoints>::New();
    polydataToProcessMinima->SetPoints( pointsMinima );
    polydataToProcessMinima->GetPointData()->SetScalars( valuesMinima );

    //configure the volume for Shepard's Method interpolation algorithm
    //vtkGaussianSplatter can be an alternative if results are not good
    vtkSmartPointer<vtkShepardMethod> shepard = vtkSmartPointer<vtkShepardMethod>::New();
    shepard->SetInputData( polydataToProcessMaxima );
    shepard->SetMaximumDistance(1); //1.0 means it uses all points (slower, but without search neighborhood artifacts)
    double xmin = m_inputGrid->getOriginX() - m_inputGrid->getCellSizeI() / 2.0;
    double ymin = m_inputGrid->getOriginY() - m_inputGrid->getCellSizeJ() / 2.0;
    double zmin = m_inputGrid->getOriginZ() - m_inputGrid->getCellSizeK() / 2.0;
    double xmax = xmin + m_inputGrid->getCellSizeI() * nI;
    double ymax = ymin + m_inputGrid->getCellSizeJ() * nJ;
    double zmax = zmin + m_inputGrid->getCellSizeK() * nK;
    shepard->SetModelBounds( xmin, xmax, ymin, ymax, zmin, zmax);
    shepard->SetSampleDimensions( nI, nJ, nK );

    vtkSmartPointer<vtkImageData> output = shepard->GetOutput();

    IJQuick3DViewer* ijq3dv = new IJQuick3DViewer( this );
    ijq3dv->show();
    ijq3dv->display( output );

    //---------------------------------------------------------------------------------------
}
