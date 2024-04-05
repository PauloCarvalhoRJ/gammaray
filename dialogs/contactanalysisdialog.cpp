#include "contactanalysisdialog.h"
#include "ui_contactanalysisdialog.h"

#include "domain/attribute.h"
#include "domain/datafile.h"
#include "domain/application.h"
#include "geostats/contactanalysis.h"
#include "widgets/categoryselector.h"
#include "widgets/linechartwidget.h"
#include "dialogs/emptydialog.h"

#include <QMessageBox>
#include <algorithm>

ContactAnalysisDialog::ContactAnalysisDialog(Attribute *attributeGrade,
                                             Attribute *attributeDomains,
                                             QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ContactAnalysisDialog),
    m_attributeGrade(attributeGrade),
    m_attributeDomains(attributeDomains)
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    //set dialog title
    setWindowTitle( "Contact Analysis" );

    //update the suffix of the lag size spin box with the length unit symbol.
    onLengthUnitSymbolChanged( ui->txtLengthUnitSymbol->text() );

    //get the pointer to the variables' parent data file and set caption of the dialog's label
    m_dataFile = dynamic_cast<DataFile*>( attributeGrade->getContainingFile() );
    DataFile* dataFileOfDomain = dynamic_cast<DataFile*>( attributeDomains->getContainingFile() );
    if( ! m_dataFile || ! dataFileOfDomain || (dataFileOfDomain != m_dataFile ) ) { //sanity check
        Application::instance()->logError( "ContactAnalysisDialog::ContactAnalysisDialog(): "
                                           "Container file object of the grade and/or domains attributes is not a data file or "
                                           "their parent files are different, which is not allowed for contact anaysis. ", true );
    } else {
        //enable/disable certain dialog widgets depending on whether the data set is 3D or is gridded.
        ui->lblDataFile->setText( ui->lblDataFile->text() + "<b>" + m_dataFile->getName() + "</b>" );
        if( m_dataFile->isGridded() || !m_dataFile->isTridimensional() ){
            ui->lblZTolerance->setEnabled( false );
            ui->dblSpinZTolerance->setEnabled( false );
        }
        if( ! m_dataFile->isTridimensional() )
            ui->radioVertical->setEnabled( false );
    }

    //display the variables' names
    ui->lblGradeAttribute->setText( ui->lblGradeAttribute->text() + "<b>" +  attributeGrade->getName() + "</b>" );
    ui->lblDomainsAttribute->setText( ui->lblDomainsAttribute->text() + "<b>" +  attributeDomains->getName() + "</b>" );

    //get the domains variable's category definition
    CategoryDefinition* cd = m_dataFile->getCategoryDefinition( attributeDomains );
    if( ! cd ){ //sanity check
        Application::instance()->logError( "ContactAnalysisDialog::ContactAnalysisDialog(): "
                                           "Category definition of domains variable is null. ", true );
    }

    //create the category selector widgets so the user can select both domains for the contact analysis
    m_selectorDomain1 = new CategorySelector( cd );
    m_selectorDomain2 = new CategorySelector( cd );
    ui->frmDomain1->layout()->addWidget( m_selectorDomain1 ); //Qt takes ownership of the widget object with this
    ui->frmDomain2->layout()->addWidget( m_selectorDomain2 ); //Qt takes ownership of the widget oject  with this
}

ContactAnalysisDialog::~ContactAnalysisDialog()
{
    delete ui;
}

void ContactAnalysisDialog::onLengthUnitSymbolChanged(QString lengthUnitSymbol)
{
    ui->dblSpinLagSize->setSuffix( lengthUnitSymbol );
}

void ContactAnalysisDialog::onProceed()
{
    ContactAnalysis contactAnalysis;
    if( ui->radioLateral->isChecked() )
        contactAnalysis.setMode( ContactAnalysisMode::LATERAL );
    else
        contactAnalysis.setMode( ContactAnalysisMode::VERTICAL );
    contactAnalysis.setLagSize( ui->dblSpinLagSize->value() );
    contactAnalysis.setZtolerance( ui->dblSpinZTolerance->value() );
    contactAnalysis.setDomain1_code( static_cast<uint16_t>(m_selectorDomain1->getSelectedCategoryCode()) );
    contactAnalysis.setDomain2_code( static_cast<uint16_t>(m_selectorDomain2->getSelectedCategoryCode()) );
    contactAnalysis.setNumberOfLags( static_cast<uint16_t>(ui->spinNumberOfLags->value()) );
    contactAnalysis.setInputDataFile( m_dataFile );
    contactAnalysis.setAttributeGrade( m_attributeGrade );
    contactAnalysis.setAttributeDomains( m_attributeDomains );
    contactAnalysis.setMaxNumberOfSamples( static_cast<uint16_t>(ui->spinMaxNumberOfSamples->value()) );
    contactAnalysis.setMinNumberOfSamples( static_cast<uint16_t>(ui->spinMinNumberOfSamples->value()) );

    if( ! contactAnalysis.run() ) {

        Application::instance()->logError("ContactAnalysisDialog::onProceed(): failed execution:");
        Application::instance()->logError("    " + contactAnalysis.getLastError());
        QMessageBox::critical( this, "Error", "Contact analysis failed.  Further details in the message panel." );

    } else {

        //shortcut for the STL's not-a-number value.
        const double NaN = std::numeric_limits<double>::quiet_NaN();

        //get the contact analysis results
        std::vector<
           std::pair<
              ContactAnalysis::Lag,
              ContactAnalysis::MeanGradesBothDomains
           >
        > results = contactAnalysis.getResults();

        //prepare data points for chart plotting
        std::vector<std::vector<double> > chartData;

        //traverse the results to fill the chart data
        for( const std::pair< ContactAnalysis::Lag, ContactAnalysis::MeanGradesBothDomains >& result : results ){
            //                      lag values     grades of domain 1   grades of domain 2
            chartData.push_back( { -result.first,  result.second.first,         0.0          } );
        }
        std::reverse( chartData.begin(), chartData.end() );
        for( const std::pair< ContactAnalysis::Lag, ContactAnalysis::MeanGradesBothDomains >& result : results ){
            //                      lag values     grades of domain 1   grades of domain 2
            chartData.push_back( {  result.first,        0.0,          result.second.second } );
        }

        //display the results
        LineChartWidget* lcw = new LineChartWidget(this);
        lcw->setData( chartData, 0 );
        EmptyDialog* ed = new EmptyDialog( this );
        ed->addWidget( lcw );
        ed->setWindowTitle( "Contact analysis results" );
        ed->show();

    }
}
