#include "driftanalysisdialog.h"
#include "ui_driftanalysisdialog.h"

#include "domain/attribute.h"
#include "domain/application.h"
#include "domain/datafile.h"
#include "geostats/driftanalysis.h"
#include "geostats/quadratic3dtrendmodelfitting.h"
#include "widgets/linechartwidget.h"
#include "viewer3d/view3dcolortables.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <thread>

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
    m_chartVertical = new LineChartWidget( /*nullptr, true*/ );
    m_chartVertical  ->setLegendVisible( false );

    ui->grpWEdrift->layout()->addWidget( m_chartWestEast );
    ui->grpSNdrift->layout()->addWidget( m_chartSouthNorth );
    ui->grpVerticalDrift->layout()->addWidget( m_chartVertical );

    //if data set is not 3D, hide the vertical drift chart
    if( ! m_dataFile->isTridimensional() )
        ui->grpVerticalDrift->hide();

    ui->grpGeneticAlgorithmParams->hide();

    //defult is the number of logical processing cores made visible by the OS
    ui->spinNumberOfThreads->setValue( std::thread::hardware_concurrency() );
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
        for( const std::pair< DriftAnalysis::coordX, DriftAnalysis::Mean >& result : resultsWestEast   )
            chartDataWestEast  .push_back( { result.first,  result.second } );
        for( const std::pair< DriftAnalysis::coordY, DriftAnalysis::Mean >& result : resultsSouthNorth )
            chartDataSouthNorth.push_back( { result.first,  result.second } );
        for( const std::pair< DriftAnalysis::coordZ, DriftAnalysis::Mean >& result : resultsVertical   )
            chartDataVertical  .push_back( { result.second,  result.first } );

        //set some properties of the domain categories relevant to make
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
                                 {{1, "Z" }},
                                 {{1, colorVertical}} );
        m_chartVertical->setXaxisCaption( "mean " + m_attribute->getName() );

    }

}

void DriftAnalysisDialog::onFitTrendModel()
{
    m_dataFile->loadData();

    //determine whether the dataset is 3D
    bool is3D = m_dataFile->isTridimensional();

    //get the data column index of the input attribute in the data file
    uint indexOfVariable = m_attribute->getAttributeGEOEASgivenIndex() - 1;

    //get no-data value info
    double NDV = m_dataFile->getNoDataValueAsDouble();
    bool hasNDV = m_dataFile->hasNoDataValue();

    //compute the orders of magnitude of a possible trend model and that of the data values
    //this is useful to present the user with initial parameter search domain in the proper scale
    double magnitude_of_trend_model = 0.0;
    double magnitude_of_data_values = 0.0;
    {
        QProgressDialog progressDialog;
        progressDialog.setRange(0, m_dataFile->getDataLineCount());
        progressDialog.setValue( 0 );
        progressDialog.show();
        progressDialog.setLabelText("Computing orders of magnitude of data values and of trend model parameters...");

        double x, y, z;
        for(uint iRow=0; iRow < m_dataFile->getDataLineCount(); iRow++ ){

            m_dataFile->getDataSpatialLocation( iRow, x, y, z );
            magnitude_of_trend_model  = x * x;
            magnitude_of_trend_model += x * y;
            magnitude_of_trend_model += is3D ? std::abs( x * z ) : 0.0;
            magnitude_of_trend_model += y * y;
            magnitude_of_trend_model += is3D ? std::abs( y * z ) : 0.0;
            magnitude_of_trend_model += is3D ? std::abs( z * z ) : 0.0;
            magnitude_of_trend_model +=     x;
            magnitude_of_trend_model +=     y;
            magnitude_of_trend_model += is3D ?  std::abs( z )    : 0.0;

            double data_value = m_dataFile->data( iRow, indexOfVariable );
            if( ! hasNDV || ! Util::almostEqual2sComplement( data_value, NDV, 1 ) )
                magnitude_of_data_values += std::abs( data_value );

            //update progress bar from time to time
            if( ! iRow % ( m_dataFile->getDataLineCount() / 100 ) ){
                progressDialog.setValue( iRow );
                QApplication::processEvents(); // let Qt update the UI
            }
        }
    }

    //compute the approx. powers of 10 (orders of magnitude (approx. power of 1000) for each case
    int exp_trend_model = static_cast<int>( std::log10<double>(magnitude_of_trend_model).real() );
    int exp_data_values = static_cast<int>( std::log10<double>(magnitude_of_data_values).real() );

    //initialize the value of the magnitude of the parameter search domain with an adequate value
    ui->spinCoeffSearchWindowSizeMagnitude->setValue     ( exp_data_values - exp_trend_model     );
    ui->spinSearchWindowShiftThresholdMagnitude->setValue( exp_data_values - exp_trend_model - 2 );

    //show the frame with the trend model fitting controls
    ui->grpGeneticAlgorithmParams->setVisible( ! ui->grpGeneticAlgorithmParams->isVisible() );
}

void DriftAnalysisDialog::onRunFitTrendModel()
{
    //fit a trend model to the data
    Quadratic3DTrendModelFitting q3dtmf( m_dataFile, m_attribute );
    Quad3DTrendModelFittingAuxDefs::Parameters modelParameters =
            q3dtmf.processWithNonLinearLeastSquares();
}

/*
void DriftAnalysisDialog::onRunFitTrendModel()
{
    //fit a trend model to the data
    Quadratic3DTrendModelFitting q3dtmf( m_dataFile, m_attribute );
    Quad3DTrendModelFittingAuxDefs::Parameters modelParameters =
            q3dtmf.processWithGenetic(
                ui->spinNumberOfThreads->value(),
                ui->spinSeed->value(),
                ui->spinNumberOfGenerations->value(),
                ui->spinPopulationSize->value(),
                ui->spinSelectionSize->value(),
                ui->dblSpinProbabilityOfCrossover->value(),
                ui->spinPointOfCrossover->value(),
                ui->dblSpinMutationRate->value(),
                ui->dblCoeffSearchWindowSize->value() * std::pow( 10, ui->spinCoeffSearchWindowSizeMagnitude->value() ),
                ui->dblSearchWindowShiftThreshold->value() * std::pow( 10, ui->spinSearchWindowShiftThresholdMagnitude->value()) );


    //find the highest and lowest parameters values for color table
    double absmax = -std::numeric_limits<double>::max();
    for( int i = 0; i < Quad3DTrendModelFittingAuxDefs::N_PARAMS; i++)
        if( std::abs(modelParameters[i]) > absmax ) absmax = std::abs(modelParameters[i]);

    //a lambda to automatize the generation of HTML to display a background color proportional to the parameter value
    auto lambdaMakeBGColor = [absmax](double value){
        return " bgcolor='" + Util::getHTMLColorFromValue( value, ColorTable::SEISMIC, -absmax, absmax ) + "'";
    };

    //a lambda to automatize the generation of HTML to render text in a contrasting color with respect to the background color
    auto lambdaMakeFontColor = [absmax](double value){
        return Util::fontColorTag( QString::number(value), Util::getColorFromValue( value, ColorTable::SEISMIC, -absmax, absmax ) );
    };

    const QString btdSansColor = "<td style='border: 0px; padding 0px;'>";
    const QString btdAvecColor = "<td style='border: 0px; padding 0px;' ";
    const QString etd = "</td>";
    QString output = "<html><head/><body><table><tr>";
    output += btdAvecColor + lambdaMakeBGColor( modelParameters.a ) + ">" + lambdaMakeFontColor(modelParameters.a) + etd
            + btdSansColor + "x<sup>2</sup> + " + etd;
    output += btdAvecColor + lambdaMakeBGColor( modelParameters.b ) + ">" + lambdaMakeFontColor(modelParameters.b) + etd
            + btdSansColor + "xy + "            + etd;
    output += btdAvecColor + lambdaMakeBGColor( modelParameters.c ) + ">" + lambdaMakeFontColor(modelParameters.c) + etd
            + btdSansColor + "xz + "            + etd;
    output += btdAvecColor + lambdaMakeBGColor( modelParameters.d ) + ">" + lambdaMakeFontColor(modelParameters.d) + etd
            + btdSansColor + "y<sup>2</sup> + " + etd;
    output += btdAvecColor + lambdaMakeBGColor( modelParameters.e ) + ">" + lambdaMakeFontColor(modelParameters.e) + etd
            + btdSansColor + "yz + "            + etd;
    output += btdAvecColor + lambdaMakeBGColor( modelParameters.f ) + ">" + lambdaMakeFontColor(modelParameters.f) + etd
            + btdSansColor + "z<sup>2</sup> + " + etd;
    output += btdAvecColor + lambdaMakeBGColor( modelParameters.g ) + ">" + lambdaMakeFontColor(modelParameters.g) + etd
            + btdSansColor + "x + "             + etd;
    output += btdAvecColor + lambdaMakeBGColor( modelParameters.h ) + ">" + lambdaMakeFontColor(modelParameters.h) + etd
            + btdSansColor + "y + "             + etd;
    output += btdAvecColor + lambdaMakeBGColor( modelParameters.i ) + ">" + lambdaMakeFontColor(modelParameters.i) + etd
            + btdSansColor + "z"                + etd;
    output += "</tr></table></body></html>";

    ui->lblTrendModel->setText( output );

    m_lastFitDriftModelParameters.reset( new Quad3DTrendModelFittingAuxDefs::Parameters( modelParameters ) );
}
*/

void DriftAnalysisDialog::onSaveNewVariableWithDriftModel()
{

    //present a variable naming dialog to the user
    QString new_variable_name = "drift_model";
    {
        bool ok;
        QString ps_file_name = QInputDialog::getText(this, "Name the new variable",
                                                     "New variable with the trend model evaluated in data locations: ",
                                                     QLineEdit::Normal, new_variable_name, &ok);
        if( ! ok )
            return;
    }

    //allocate container for the drift values
    std::vector<double> drift_values( m_dataFile->getDataLineCount() );

    //for each data sample
    for( uint64_t iRow = 0; iRow < m_dataFile->getDataLineCount(); iRow++){

        //get the spatial location of the current sample
        double x, y, z;
        m_dataFile->getDataSpatialLocation( iRow, x, y, z );

        //evaluate the trend model at the current sample location
        double drift_value =
                m_lastFitDriftModelParameters->a * x * x +
                m_lastFitDriftModelParameters->b * x * y +
                m_lastFitDriftModelParameters->c * x * z +
                m_lastFitDriftModelParameters->d * y * y +
                m_lastFitDriftModelParameters->e * y * z +
                m_lastFitDriftModelParameters->f * z * z +
                m_lastFitDriftModelParameters->g * x +
                m_lastFitDriftModelParameters->h * y +
                m_lastFitDriftModelParameters->i * z ;

        //store the trend model value at the current sample location
        drift_values[iRow] = drift_value;
    }

    //adds the drift values as a new attribute to the data set file
    m_dataFile->addNewDataColumn( new_variable_name, drift_values );

}
