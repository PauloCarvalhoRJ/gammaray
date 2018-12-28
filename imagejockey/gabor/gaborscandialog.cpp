#include "gaborscandialog.h"
#include "ui_gaborscandialog.h"
#include "imagejockey/gabor/gaborutils.h"
#include "imagejockey/widgets/ijgridviewerwidget.h"
#include "imagejockey/svd/svdfactor.h"
#include "spectral/spectral.h"
#include <itkStatisticsImageFilter.h>
#include <itkAbsImageFilter.h>
#include <QProgressDialog>

GaborScanDialog::GaborScanDialog(IJAbstractCartesianGrid *inputGrid,
                                 uint inputVariableIndex,
                                 double meanMajorAxis,
                                 double meanMinorAxis,
                                 double sigmaMajorAxis,
                                 double sigmaMinorAxis,
                                 int kernelSizeI,
                                 int kernelSizeJ,
                                 QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GaborScanDialog),
    m_inputGrid( inputGrid ),
    m_inputVariableIndex( inputVariableIndex ),
    m_meanMajorAxis( meanMajorAxis ),
    m_meanMinorAxis( meanMinorAxis ),
    m_sigmaMajorAxis( sigmaMajorAxis ),
    m_sigmaMinorAxis( sigmaMinorAxis ),
    m_kernelSizeI( kernelSizeI ),
    m_kernelSizeJ( kernelSizeJ )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle( "Scan Gabor Response Dialog" );


    m_ijgv = new IJGridViewerWidget( true, false, false );
    ui->frmGridDisplay->layout()->addWidget( m_ijgv );
}

GaborScanDialog::~GaborScanDialog()
{
    delete ui;
}

void GaborScanDialog::updateFrequAzSelectionDisplay()
{
    QString output = "<html><head/><body><table>";
    output += "<tr><td><b>f. min.</b>"
              "</td><td><b>f. max.</b>"
              "</td><td><b>az. min.</b>"
              "</td><td><b>az. max.</b></td></tr>";
    for( GaborFrequencyAzimuthSelection fAzSel : m_freqAzSelections )
        output += "<tr><td><center>" + QString::number( fAzSel.minF ) +
                  "</center></td><td><center>" + QString::number( fAzSel.maxF ) +
                  "</center></td><td><center>" + QString::number( fAzSel.minAz ) +
                  "</center></td><td><center>" + QString::number( fAzSel.maxAz ) + "</center></td></tr>";
    output += "</table></body></html>";
    ui->lblSelectionDisplay->setText( output );
}

void GaborScanDialog::onScan()
{
    typedef itk::StatisticsImageFilter<GaborUtils::ImageType> StatisticsImageFilterType;
    typedef itk::AbsImageFilter <GaborUtils::ImageType, GaborUtils::ImageType> AbsImageFilterType;

    // do not set limits lesser than 3.0 or greater than 177.0 degrees
    // Otherwise The transform object used to manipulate the kernel crashes (figures).
    double az0 = 3.0;
    double az1 = 177.0;

    //get the user settings
    double azStep = ui->txtAzStep->text().toDouble();
    double fStep = ui->txtFStep->text().toDouble();
    double f0 = ui->txtF0->text().toDouble();
    double f1 = ui->txtF1->text().toDouble();

    //convert the input data to ITK image
    GaborUtils::ImageTypePtr inputImage =
            GaborUtils::createITKImageFromCartesianGrid( *m_inputGrid, m_inputVariableIndex );

    //define the list of azimuths to scan
    std::vector<double> azSchedule;
    for( double azimuth = az0; azimuth <= az1; azimuth += azStep )
        azSchedule.push_back( azimuth );

    //define the list of frequencies to scan
    std::vector<double> fSchedule;
    for( double frequency = f0; frequency <= f1; frequency += fStep )
        fSchedule.push_back( frequency );

    //////////////////////////////////
    QProgressDialog progressDialog;
    progressDialog.show();
    progressDialog.setLabelText("Scanning responses of Gabor frequencies and azimuths...");
    progressDialog.setMinimum( 0 );
    progressDialog.setMaximum( azSchedule.size() * fSchedule.size() );
    progressDialog.show();
    /////////////////////////////////

    //create a grid object to receive the metric values during the scan
    spectral::array gridData( static_cast<spectral::index>(fSchedule.size()),
                              static_cast<spectral::index>(azSchedule.size()) );


    //scan frequencies and azimuths
    StatisticsImageFilterType::Pointer statisticsImageFilter = StatisticsImageFilterType::New();
    AbsImageFilterType::Pointer absFilter = AbsImageFilterType::New();
    spectral::index iAz = 0;
    const int MEAN = 0;
    const int MAX = 1;
    int whichMetric = 2;
    if( ui->cmbMetric->currentText() == "mean" )
        whichMetric = MEAN;
    if( ui->cmbMetric->currentText() == "maximum" )
        whichMetric = MAX;
    for( const double& azimuth : azSchedule ){
        spectral::index iF = 0;
        for( const double& frequency : fSchedule ){
            //update the progress dialog
            progressDialog.setValue( progressDialog.value()+1 );
            QApplication::processEvents();
            //compute the Gabor response of a given frequency-azimuth pair
            GaborUtils::ImageTypePtr response =
                    GaborUtils::computeGaborResponse( frequency,
                                                      azimuth,
                                                      m_meanMajorAxis,
                                                      m_meanMinorAxis,
                                                      m_sigmaMajorAxis,
                                                      m_sigmaMinorAxis,
                                                      m_kernelSizeI,
                                                      m_kernelSizeJ,
                                                      inputImage );
            //get the absolute values from the response grid
            absFilter->SetInput( response );
            absFilter->Update();
            //compute the image (absolute values) stats
            statisticsImageFilter->SetInput( absFilter->GetOutput() );
            statisticsImageFilter->Update();
            //get the metric
            double metric;
            switch( whichMetric ){
            case MEAN: metric = statisticsImageFilter->GetMean(); break;
            case MAX: metric = statisticsImageFilter->GetMaximum(); break;
            default: metric = 0.0;
            }

            gridData(iF, iAz) = metric;
            ++iF;
        }
        ++iAz;
    }

    //show the scan result
    SVDFactor* grid = new SVDFactor( std::move(gridData),
                                     1, 0.42, f0, az0, 0.0, fStep, azStep, 1.0, 0.0 );
    m_ijgv->setFactor( grid );
}

void GaborScanDialog::onAddSelection()
{
    m_freqAzSelections.push_back( { ui->txtSelFmin->text().toDouble(),
                                    ui->txtSelFmax->text().toDouble(),
                                    ui->txtSelAzMin->text().toDouble(),
                                    ui->txtSelAzMax->text().toDouble() } );
    updateFrequAzSelectionDisplay();
}

void GaborScanDialog::onClearSelectionList()
{
    m_freqAzSelections.clear();
    updateFrequAzSelectionDisplay();
}
