#include "driftanalysisdialog.h"
#include "ui_driftanalysisdialog.h"

#include "domain/attribute.h"
#include "domain/application.h"
#include "domain/datafile.h"
#include "geostats/driftanalysis.h"
#include "widgets/linechartwidget.h"

#include <QMessageBox>

DriftAnalysisDialog::DriftAnalysisDialog(DataFile *dataFile, Attribute *attribute, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DriftAnalysisDialog),
    m_dataFile(dataFile),
    m_attribute(attribute)
{
    ui->setupUi(this);

    this->setWindowTitle("Drift Analysis");

    ui->lblDataFile->setText("<b>" + m_dataFile->getName() + "</b>");
    ui->lblAttribute->setText("<b>" + m_attribute->getName() + "</b>");

    m_chartWestEast = new LineChartWidget();
    m_chartWestEast  ->setLegendVisible( false );
    m_chartSouthNorth = new LineChartWidget();
    m_chartSouthNorth->setLegendVisible( false );
    m_chartVertical = new LineChartWidget(nullptr, true);
    m_chartVertical  ->setLegendVisible( false );

    ui->grpWEdrift->layout()->addWidget( m_chartWestEast );
    ui->grpSNdrift->layout()->addWidget( m_chartSouthNorth );
    ui->grpVerticalDrift->layout()->addWidget( m_chartVertical );

    //if data set is not 3D, hide the vertical drift chart
    if( ! m_dataFile->isTridimensional() )
        ui->grpVerticalDrift->hide();
}

DriftAnalysisDialog::~DriftAnalysisDialog()
{
    delete ui;
}

void DriftAnalysisDialog::onRun()
{
    DriftAnalysis driftAnalysis;
    driftAnalysis.setAttribute( m_attribute );
    driftAnalysis.setInputDataFile( m_dataFile );
    driftAnalysis.setNumberOfSteps( ui->spinNumberOfSteps->value() );

    if( ! driftAnalysis.run() ) {

        Application::instance()->logError("DriftAnalysisDialog::onRun(): failed execution:");
        Application::instance()->logError("    " + driftAnalysis.getLastError());
        QMessageBox::critical( this, "Error", "Drift analysis failed.  Further details in the message panel." );

    } else { //if the drift analysis completed successfully

        //shortcut for the STL's not-a-number value.
        const double NaN = std::numeric_limits<double>::quiet_NaN();

        //get the drift analysis results
         std::vector< std::pair< DriftAnalysis::coordX, DriftAnalysis::Mean > >
                 resultsWestEast = driftAnalysis.getResultsWestEast();
         std::vector< std::pair< DriftAnalysis::coordY, DriftAnalysis::Mean > >
                 resultsSouthNorth = driftAnalysis.getResultsSouthNorth();
         std::vector< std::pair< DriftAnalysis::coordZ, DriftAnalysis::Mean > >
                 resultsVertical = driftAnalysis.getResultsVertical();

        //prepare data points for the three chart plotting
        //outer vector: the series of multivariate data
        //inner vectors: each multivariate datum (each element is a X, Y1, Y2,... value).
        std::vector< std::vector<double> > chartDataWestEast;
        std::vector< std::vector<double> > chartDataSouthNorth;
        std::vector< std::vector<double> > chartDataVertical;

        //traverse the results to fill the three chart data vectors
        for( const std::pair< DriftAnalysis::coordX, DriftAnalysis::Mean >& result : resultsWestEast )
            chartDataWestEast.push_back( { result.first,  result.second } );
        for( const std::pair< DriftAnalysis::coordY, DriftAnalysis::Mean >& result : resultsSouthNorth )
            chartDataWestEast.push_back( { result.first,  result.second } );
        for( const std::pair< DriftAnalysis::coordZ, DriftAnalysis::Mean >& result : resultsVertical )
            chartDataWestEast.push_back( { result.first,  result.second } );

        //get some properties of the domain categories relevant to make
        //the chart informative
        QColor colorWestEast   = QColorConstants::Red;
        QColor colorSouthNorth = QColorConstants::DarkGreen;
        QColor colorVertical   = QColorConstants::DarkBlue;

        //display the results
        m_chartWestEast->setData( chartDataWestEast, 0,
                                 {{}},
                                 {{1, "mean " + m_attribute->getName() }},
                                 {{1, colorWestEast}} );
        m_chartWestEast->setXaxisCaption( "Easting" );
        m_chartSouthNorth->setData( chartDataSouthNorth, 0,
                                 {{}},
                                 {{1, "mean " + m_attribute->getName() }},
                                 {{1, colorSouthNorth}} );
        m_chartSouthNorth->setXaxisCaption( "Northing" );
        m_chartVertical->setData( chartDataVertical, 0,
                                 {{}},
                                 {{1, "mean " + m_attribute->getName() }},
                                 {{1, colorVertical}} );
        m_chartVertical->setXaxisCaption( "Z" );

    }
}
