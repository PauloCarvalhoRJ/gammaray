#include "driftanalysisdialog.h"
#include "ui_driftanalysisdialog.h"

#include "domain/attribute.h"
#include "domain/application.h"
#include "domain/datafile.h"
#include "geostats/driftanalysis.h"
#include "geostats/quadratic3dtrendmodelfitting.h"
#include "widgets/linechartwidget.h"
#include "viewer3d/view3dcolortables.h"
#include "gslib/gslibparametersdialog.h"
#include "gslib/gslibparameterfiles/kt3dtrendmodelparameters.h"

#include <QClipboard>
#include <QInputDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <thread>

DriftAnalysisDialog::DriftAnalysisDialog(DataFile *dataFile, Attribute *attribute, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DriftAnalysisDialog),
    m_dataFile(dataFile),
    m_attribute(attribute),
    m_lastNameForDriftVariable("drift_model")
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

    ui->grpFitTrendModel->hide();
}

DriftAnalysisDialog::~DriftAnalysisDialog()
{
    delete ui;
}

void DriftAnalysisDialog::performDriftAnalysis( DriftAnalysis& driftAnalysis, bool clear_curves )
{
    if( ! driftAnalysis.run() ) {

        Application::instance()->logError("DriftAnalysisDialog::performDriftAnalysis(): failed execution:");
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

        QPen style;
        if( ! clear_curves ) //user wants to display drift model along observed drift
            style.setStyle( Qt::DashLine );

        //display the results index 0: X values; 1: Y values
        m_chartWestEast->setData( chartDataWestEast, 0, clear_curves,
                                 {{}},
                                 {{1, "mean " + m_attribute->getName() }},
                                 {{1, colorWestEast}},
                                 {{1, style}} );
        m_chartWestEast->setXaxisCaption( "Easting" );
        m_chartSouthNorth->setData( chartDataSouthNorth, 0, clear_curves,
                                 {{}},
                                 {{1, "mean " + m_attribute->getName() }},
                                 {{1, colorSouthNorth}},
                                 {{1, style}} );
        m_chartSouthNorth->setXaxisCaption( "Northing" );
        m_chartVertical->setData( chartDataVertical, 0, clear_curves,
                                 {{}},
                                 {{1, "Z" }},
                                 {{1, colorVertical}},
                                 {{1, style}} );
        m_chartVertical->setXaxisCaption( "mean " + m_attribute->getName() );

    }

}

void DriftAnalysisDialog::displayParamaters(const Quad3DTrendModelFittingAuxDefs::Parameters &model_parameters)
{
    //find the highest and lowest parameters values for color table
    double absmax = -std::numeric_limits<double>::max();
    for( int i = 0; i < Quad3DTrendModelFittingAuxDefs::N_PARAMS; i++)
        if( std::abs(model_parameters[i]) > absmax ) absmax = std::abs(model_parameters[i]);

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
    output += btdAvecColor + lambdaMakeBGColor( model_parameters.a ) + ">" + lambdaMakeFontColor(model_parameters.a) + etd
            + btdSansColor + "x<sup>2</sup> + " + etd;
    output += btdAvecColor + lambdaMakeBGColor( model_parameters.b ) + ">" + lambdaMakeFontColor(model_parameters.b) + etd
            + btdSansColor + "xy + "            + etd;
    output += btdAvecColor + lambdaMakeBGColor( model_parameters.c ) + ">" + lambdaMakeFontColor(model_parameters.c) + etd
            + btdSansColor + "xz + "            + etd;
    output += btdAvecColor + lambdaMakeBGColor( model_parameters.d ) + ">" + lambdaMakeFontColor(model_parameters.d) + etd
            + btdSansColor + "y<sup>2</sup> + " + etd;
    output += btdAvecColor + lambdaMakeBGColor( model_parameters.e ) + ">" + lambdaMakeFontColor(model_parameters.e) + etd
            + btdSansColor + "yz + "            + etd;
    output += btdAvecColor + lambdaMakeBGColor( model_parameters.f ) + ">" + lambdaMakeFontColor(model_parameters.f) + etd
            + btdSansColor + "z<sup>2</sup> + " + etd;
    output += btdAvecColor + lambdaMakeBGColor( model_parameters.g ) + ">" + lambdaMakeFontColor(model_parameters.g) + etd
            + btdSansColor + "x + "             + etd;
    output += btdAvecColor + lambdaMakeBGColor( model_parameters.h ) + ">" + lambdaMakeFontColor(model_parameters.h) + etd
            + btdSansColor + "y + "             + etd;
    output += btdAvecColor + lambdaMakeBGColor( model_parameters.i ) + ">" + lambdaMakeFontColor(model_parameters.i) + etd
            + btdSansColor + "z"                + etd;
    output += "</tr></table></body></html>";

    ui->lblTrendModel->setText( output );

}

void DriftAnalysisDialog::onRun()
{
    DriftAnalysis driftAnalysis;
    driftAnalysis.setAttribute( m_attribute );
    driftAnalysis.setInputDataFile( m_dataFile );
    driftAnalysis.setNumberOfSteps( ui->spinNumberOfSteps->value() );

    performDriftAnalysis( driftAnalysis );
}

void DriftAnalysisDialog::onFitTrendModel()
{
    //toggle the frame with the trend model fitting controls
    ui->grpFitTrendModel->setVisible( ! ui->grpFitTrendModel->isVisible() );
}

void DriftAnalysisDialog::onRunFitTrendModel()
{
    //fit a trend model to the data
    Quadratic3DTrendModelFitting q3dtmf( m_dataFile, m_attribute );
    Quad3DTrendModelFittingAuxDefs::Parameters modelParameters =
            q3dtmf.processWithNonLinearLeastSquares();

    //update the trend model disaply in this dialog
    displayParamaters( modelParameters );

    //keeps the last trend model fit for further use while another one is not fit
    m_lastFitDriftModelParameters.reset( new Quad3DTrendModelFittingAuxDefs::Parameters( modelParameters ) );
}

void DriftAnalysisDialog::onSaveNewVariableWithDriftModel()
{
    if( ! m_lastFitDriftModelParameters ) {
        Application::instance()->logError("DriftAnalysisDialog::onSaveNewVariableWithDriftModel(): null drift model.  Run model fitting at least once.", true);
        return;
    }

    // make sure the data is loaded from filesystem
    m_dataFile->loadData();

    //present a variable naming dialog to the user
    {
        bool ok;
        QString var_name = QInputDialog::getText(this, "Name the new variable",
                                                     "New variable with the trend model evaluated in data locations: ",
                                                     QLineEdit::Normal, m_lastNameForDriftVariable, &ok);
        if( ! ok )
            return;
        else
            m_lastNameForDriftVariable = var_name;
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

    //add or update the trend model values in the data files
    uint driftVatGEOEASindex = m_dataFile->getFieldGEOEASIndex( m_lastNameForDriftVariable );
    uint drift_col_index = 9999;
    if( driftVatGEOEASindex == 0 ) { //the variable does not exist
        //adds the drift values as a new attribute to the data set file
        drift_col_index = m_dataFile->addNewDataColumn( m_lastNameForDriftVariable, drift_values );
    } else { //drift variable already exists in the file
        drift_col_index = driftVatGEOEASindex - 1;
        //update the data file with the recomputed drift model values
        for( uint iRow = 0;  iRow < m_dataFile->getDataLineCount(); iRow++ )
            m_dataFile->setData( iRow, drift_col_index, drift_values[iRow] );
        m_dataFile->writeToFS();
    }

    //gets the new Attribute object associated with the newly added variable with the trend model values
    Attribute* driftVariable = m_dataFile->getAttributeFromGEOEASIndex( drift_col_index + 1 );

    //sanity check
    if( ! driftVariable ){
        Application::instance()->logError( "DriftAnalysisDialog::onSaveNewVariableWithDriftModel(): failed to retrieve the Attribute object"
                                           " associated with the newly computed trend model values." );
        return;
    }

    //run drift analysis on the drift itself so the use can assess the quality of the fit
    Attribute* tmp = m_attribute;
    {
        m_attribute = driftVariable;
        DriftAnalysis driftAnalysis;
        driftAnalysis.setAttribute( driftVariable );
        driftAnalysis.setInputDataFile( m_dataFile );
        driftAnalysis.setNumberOfSteps( ui->spinNumberOfSteps->value() );
        performDriftAnalysis( driftAnalysis, false );
    }
    m_attribute = tmp;
}

void DriftAnalysisDialog::onCopyTrendModelAsCalcScript(){

    if( ! m_lastFitDriftModelParameters ) {
        Application::instance()->logError("DriftAnalysisDialog::onCopyTrendModelAsCalcScript(): null drift model.  Run model fitting at least once.", true);
        return;
    }

    // assemble calculator script
    QString script;
    script += " [output variable] := (" + QString::number(m_lastFitDriftModelParameters->a) + ") * X_ * X_ + \n";
    script += "                      (" + QString::number(m_lastFitDriftModelParameters->b) + ") * X_ * Y_ + \n";
    script += "                      (" + QString::number(m_lastFitDriftModelParameters->c) + ") * X_ * Z_ + \n";
    script += "                      (" + QString::number(m_lastFitDriftModelParameters->d) + ") * Y_ * Y_ + \n";
    script += "                      (" + QString::number(m_lastFitDriftModelParameters->e) + ") * Y_ * Z_ + \n";
    script += "                      (" + QString::number(m_lastFitDriftModelParameters->f) + ") * Z_ * Z_ + \n";
    script += "                      (" + QString::number(m_lastFitDriftModelParameters->g) + ") * X_ + \n";
    script += "                      (" + QString::number(m_lastFitDriftModelParameters->h) + ") * Y_ + \n";
    script += "                      (" + QString::number(m_lastFitDriftModelParameters->i) + ") * Z_ ; \n";

    // copy script to clipboard (CTRL+C)
    QClipboard *clipboard = QGuiApplication::clipboard();
    QString originalText = clipboard->text();
    clipboard->setText(script);
    QMessageBox::information(this, "Information", "Calculator script copied to the clipboard.");
}

void DriftAnalysisDialog::onEditTrendModelParameters()
{
    if( ! m_lastFitDriftModelParameters ) {
        Application::instance()->logError("DriftAnalysisDialog::onEditTrendModelParameters(): null drift model.  Run model fitting at least once.", true);
        return;
    }

    // open a parameter editor dialog
    Kt3dTrendModelParameters kt3dTrendModelParameters;
    kt3dTrendModelParameters.setA( m_lastFitDriftModelParameters->a );
    kt3dTrendModelParameters.setB( m_lastFitDriftModelParameters->b );
    kt3dTrendModelParameters.setC( m_lastFitDriftModelParameters->c );
    kt3dTrendModelParameters.setD( m_lastFitDriftModelParameters->d );
    kt3dTrendModelParameters.setE( m_lastFitDriftModelParameters->e );
    kt3dTrendModelParameters.setF( m_lastFitDriftModelParameters->f );
    kt3dTrendModelParameters.setG( m_lastFitDriftModelParameters->g );
    kt3dTrendModelParameters.setH( m_lastFitDriftModelParameters->h );
    kt3dTrendModelParameters.setI( m_lastFitDriftModelParameters->i );
    GSLibParametersDialog gpd( &kt3dTrendModelParameters, this );
    gpd.setWindowTitle( "Edit kt3d's trend model parameters" );

    //show the dialog modally an treat the user response
    int ret = gpd.exec();
    if( ret != QDialog::Accepted )
        return;

    //if user didn't dismiss the dialog, assign the entered values to the model
    m_lastFitDriftModelParameters->a = kt3dTrendModelParameters.getA();
    m_lastFitDriftModelParameters->b = kt3dTrendModelParameters.getB();
    m_lastFitDriftModelParameters->c = kt3dTrendModelParameters.getC();
    m_lastFitDriftModelParameters->d = kt3dTrendModelParameters.getD();
    m_lastFitDriftModelParameters->e = kt3dTrendModelParameters.getE();
    m_lastFitDriftModelParameters->f = kt3dTrendModelParameters.getF();
    m_lastFitDriftModelParameters->g = kt3dTrendModelParameters.getG();
    m_lastFitDriftModelParameters->h = kt3dTrendModelParameters.getH();
    m_lastFitDriftModelParameters->i = kt3dTrendModelParameters.getI();

    //update the model display in this dialog
    displayParamaters( *m_lastFitDriftModelParameters );
}
