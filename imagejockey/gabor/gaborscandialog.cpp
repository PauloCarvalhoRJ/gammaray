#include "gaborscandialog.h"
#include "ui_gaborscandialog.h"
#include "imagejockey/gabor/gaborutils.h"
#include "imagejockey/widgets/ijgridviewerwidget.h"
#include "imagejockey/svd/svdfactor.h"
#include "spectral/spectral.h"
#include <itkStatisticsImageFilter.h>
#include <itkAbsImageFilter.h>
#include <QProgressDialog>

GaborScanDialog::GaborScanDialog(IJAbstractCartesianGrid *inputGrid, uint inputVariableIndex,
                                 QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GaborScanDialog),
    m_inputGrid( inputGrid ),
    m_inputVariableIndex( inputVariableIndex )
{
    ui->setupUi(this);
    m_ijgv = new IJGridViewerWidget( true, false, false );
    ui->frmGridDisplay->layout()->addWidget( m_ijgv );
}

GaborScanDialog::~GaborScanDialog()
{
    delete ui;
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
    for( const double& azimuth : azSchedule ){
        spectral::index iF = 0;
        for( const double& frequency : fSchedule ){
            //update the progress dialog
            progressDialog.setValue( progressDialog.value()+1 );
            QApplication::processEvents();
            //compute the Gabor response of a given frequency-azimuth pair
            GaborUtils::ImageTypePtr response =
                    GaborUtils::computeGaborResponse( frequency, azimuth, inputImage );
            //get the absolute values from the response grid
            absFilter->SetInput( response );
            absFilter->Update();
            //compute the image (absolute values) stats
            statisticsImageFilter->SetInput( absFilter->GetOutput() );
            statisticsImageFilter->Update();
            //get the metric
            double metric;
            metric = statisticsImageFilter->GetMean();
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
