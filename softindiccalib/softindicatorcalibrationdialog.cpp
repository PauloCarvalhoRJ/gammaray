#include "softindicatorcalibrationdialog.h"
#include "ui_softindicatorcalibrationdialog.h"

#include "domain/application.h"
#include "domain/attribute.h"
#include "domain/file.h"
#include "domain/datafile.h"
#include "domain/categorydefinition.h"
#include "domain/thresholdcdf.h"
#include "domain/categorypdf.h"
#include "widgets/fileselectorwidget.h"
#include "softindicatorcalibplot.h"
#include "util.h"

#include <QHBoxLayout>

SoftIndicatorCalibrationDialog::SoftIndicatorCalibrationDialog(Attribute *at, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SoftIndicatorCalibrationDialog),
    m_at( at ),
    m_softIndCalibPlot( nullptr )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle("Soft indicator calibration for " + at->getContainingFile()->getName() + "/" + at->getName());

    //add a category definition file selection drop down menu
    m_fsw = new FileSelectorWidget( FileSelectorType::CDsCDFsandPDFs, true );
    ui->frmTopBar->layout()->addWidget( m_fsw );
    connect( m_fsw, SIGNAL(fileSelected(File*)),
             this, SLOT(onUpdateNumberOfCalibrationCurves()) );

    //add a spacer for better layout
    QHBoxLayout *hl = (QHBoxLayout*)(ui->frmTopBar->layout());
    hl->addStretch();

    //add the widget used to edit the calibration curves
    m_softIndCalibPlot = new SoftIndicatorCalibPlot(this);
    ui->frmCalib->layout()->addWidget( m_softIndCalibPlot );

    //get the Attribute's data file
    File *file = m_at->getContainingFile();
    if( file->isDataFile() ){
        DataFile *dataFile = (DataFile*)file;
        //load the data
        dataFile->loadData();
        //create an array of doubles to store the Attribute's values
        std::vector<double> data;
        // get the total number of file records
        uint nData = dataFile->getDataLineCount();
        //pre-allocate the array of doubles
        data.reserve( nData );
        //get the Attribute's GEO-EAS index
        uint atGEOEASIndex = m_at->getAttributeGEOEASgivenIndex();
        //fills the array of doubles with the data from file
        for( uint i = 0; i < nData; ++i){
            double value = dataFile->data( i, atGEOEASIndex-1 );
            //adds only if the value is not a No-Data-Value
            if( ! dataFile->isNDV( value ) )
                data.push_back( value );
        }
        //move the array of doubles to the widget (it'll no longer be availabe here)
        m_softIndCalibPlot->transferData( data );
        //set the variable name as the x-axis label.
        m_softIndCalibPlot->setXAxisLabel( at->getName() );
    }

    adjustSize();
}

SoftIndicatorCalibrationDialog::~SoftIndicatorCalibrationDialog()
{
    delete ui;
    Application::instance()->logInfo("SoftIndicatorCalibrationDialog destroyed.");
}

void SoftIndicatorCalibrationDialog::onUpdateNumberOfCalibrationCurves()
{
    File *selectedFile = m_fsw->getSelectedFile();
    if( selectedFile ){
        selectedFile->readFromFS();
        int nCategories = selectedFile->getContentsCount();
        if( selectedFile->getFileType() == "THRESHOLDCDF"){
            m_softIndCalibPlot->setNumberOfCurves( selectedFile->getContentsCount() );
            //sets the curve labels with each threshold
            ThresholdCDF *cdf = (ThresholdCDF*)selectedFile;
            for( int i = 0; i < nCategories; ++i ){
                m_softIndCalibPlot->setCurveLabel(i, "thr. = " + QString::number( cdf->get1stValue( i ), 'g', 12 ) );
                QRgb rgb = static_cast<QRgb>( (uint)( (double) std::rand() / (double)RAND_MAX * (1U<<31)) );
                m_softIndCalibPlot->setCurveColor( i, QColor( rgb ));
                m_softIndCalibPlot->setCurveBase( i, cdf->get2ndValue(i) * 100.0);
            }
        }else if( selectedFile->getFileType() == "CATEGORYDEFINITION"){
            //for categorical variables the calibration curves separate the categories, thus -1.
            m_softIndCalibPlot->setNumberOfCurves( nCategories-1 );
            //fills the areas between the curves with the colors of the categories
            CategoryDefinition *cd = (CategoryDefinition*)selectedFile;
            for( int i = 0; i < nCategories; ++i ){
                m_softIndCalibPlot->fillColor( Util::getGSLibColor( cd->getColorCode(i) ) ,
                                               i-1,
                                               cd->getCategoryName( i ));
            }
        }else if( selectedFile->getFileType() == "CATEGORYPDF"){
            //for categorical variables the calibration curves separate the categories, thus -1.
            m_softIndCalibPlot->setNumberOfCurves( nCategories-1 );
            //fills the areas between the curves with the colors of the categories
            CategoryPDF *pdf = (CategoryPDF*)selectedFile;
            CategoryDefinition *cd = pdf->getCategoryDefinition();
            cd->loadTriplets();
            double curveBase = 0.0;
            for( int i = 0; i < nCategories; ++i ){
                m_softIndCalibPlot->fillColor( Util::getGSLibColor( cd->getCategoryColorByCode( pdf->get1stValue( i ) ) ) ,
                                               i-1,
                                               cd->getCategoryNameByCode( pdf->get1stValue( i ) ) );
                if( i > 0 )
                    m_softIndCalibPlot->setCurveBase( i-1, curveBase * 100.0);
                curveBase += pdf->get2ndValue( i );
            }
            m_softIndCalibPlot->updateFillAreas();
        }
    }
}
