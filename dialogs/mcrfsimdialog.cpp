#include "mcrfsimdialog.h"
#include "ui_mcrfsimdialog.h"

#include <QDragEnterEvent>
#include <QMimeData>
#include <QMessageBox>
#include <thread> //for std::thread::hardware_concurrency()

#include "domain/application.h"
#include "domain/projectcomponent.h"
#include "domain/attribute.h"
#include "domain/project.h"
#include "domain/categorydefinition.h"
#include "domain/datafile.h"
#include "domain/cartesiangrid.h"
#include "domain/categorypdf.h"
#include "domain/verticaltransiogrammodel.h"
#include "widgets/fileselectorwidget.h"
#include "widgets/variableselector.h"
#include "widgets/cartesiangridselector.h"
#include "gslib/gslibparametersdialog.h"
#include "gslib/gslibparameterfiles/commonsimulationparameters.h"
#include "geostats/mcrfsim.h"

MCRFSimDialog::MCRFSimDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MCRFSimDialog),
    m_primFileSelector( nullptr ),
    m_primVarSelector( nullptr ),
    m_simGridSelector( nullptr ),
    m_verticalTransiogramSelector( nullptr ),
    m_globalPDFSelector( nullptr ),
    m_gradationalFieldVarSelector( nullptr ),
    m_commonSimulationParameters( new CommonSimulationParameters() )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle( "Markov Chains Random Field Simulation" );

    connect( ui->chkUseProbabilityFields, SIGNAL(clicked()), this, SLOT(onRemakeProbabilityFieldsCombos()));

    m_primFileSelector = new FileSelectorWidget( FileSelectorType::PointAndSegmentSets );
    ui->frmCmbPrimFile->layout()->addWidget( m_primFileSelector );

    m_primVarSelector = new VariableSelector( false, VariableSelectorType::CATEGORICAL );
    ui->frmCmbPrimVar->layout()->addWidget( m_primVarSelector );
    connect( m_primFileSelector, SIGNAL(dataFileSelected(DataFile*)),
             m_primVarSelector,  SLOT(onListVariables(DataFile*)) );
    connect( m_primVarSelector,  SIGNAL(currentIndexChanged(int)),
             this,               SLOT(onPrimaryVariableChanged()) );

    m_primGradationValueSelector = new VariableSelector();
    ui->frmPrimGradationValue->layout()->addWidget( m_primGradationValueSelector );
    connect( m_primFileSelector,           SIGNAL(dataFileSelected(DataFile*)),
             m_primGradationValueSelector, SLOT(onListVariables(DataFile*))  );

    //calling this slot causes the variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_primFileSelector->onSelection( 0 );

    //The list with existing cartesian grids in the project for the secondary data.
    m_simGridSelector = new CartesianGridSelector( );
    ui->frmSimGrid->layout()->addWidget( m_simGridSelector );

    m_verticalTransiogramSelector = new FileSelectorWidget( FileSelectorType::VerticalTransiogramModels );
    ui->frmTransiogramSelector->layout()->addWidget( m_verticalTransiogramSelector );

    m_globalPDFSelector = new FileSelectorWidget( FileSelectorType::PDFs );
    ui->frmCmbGlobalPDFselector->layout()->addWidget( m_globalPDFSelector );

    m_gradationalFieldVarSelector = new VariableSelector( );
    ui->frmCmbLatGradFieldSelector->layout()->addWidget( m_gradationalFieldVarSelector );
    connect( m_simGridSelector,             SIGNAL(cartesianGridSelected(DataFile*)),
             m_gradationalFieldVarSelector,   SLOT(onListVariables(DataFile*)) );

    //calling this slot causes the sec. variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_simGridSelector->onSelection( 0 );

    //init the spin box for the number of threads with the number of logical processors
    ui->spinNumberOfThreads->setValue( std::thread::hardware_concurrency() );

    onRemakeProbabilityFieldsCombos();
}

MCRFSimDialog::~MCRFSimDialog()
{
    delete ui;
    delete m_commonSimulationParameters;
}

void MCRFSimDialog::onRemakeProbabilityFieldsCombos()
{
    //removes the current comboboxes
    for( VariableSelector* vs : m_probFieldsSelectors )
       delete vs; //this pointer is managed by Qt, so this causes its detachement from the dialog's UI.
    m_probFieldsSelectors.clear();

    if( ! m_simGridSelector ){
        Application::instance()->logWarn("MCRFSimDialog::onRemakeProbabilityFieldsCombos(): pointer to simulation grid selector is null. Ignoring.");
        return;
    }

    if( ! ui->chkUseProbabilityFields->isChecked() )
        return;

    //create new ones for each category
    DataFile* df = static_cast<DataFile*>( m_primFileSelector->getSelectedFile() );
    if( df ){
        Attribute* at = m_primVarSelector->getSelectedVariable();
        if( at ){
            CategoryDefinition* cd = df->getCategoryDefinition( at );
            if( cd ){
                cd->loadQuintuplets(); //loads the C.D. data from the filesystem.
                for( uint iCatIndex = 0; iCatIndex < cd->getCategoryCount(); ++iCatIndex ){
                    VariableSelector* probFieldSelector = new VariableSelector( );
                    probFieldSelector->setCaption( "   " + cd->getCategoryName( iCatIndex ) + "   " );
                    probFieldSelector->setCaptionBGColor( cd->getCustomColor( iCatIndex ) );
                    ui->grpBoxSecondaryData->layout()->addWidget( probFieldSelector );
                    connect( m_simGridSelector, SIGNAL(cartesianGridSelected(DataFile*)),
                             probFieldSelector, SLOT(onListVariables(DataFile*)) );
                    m_probFieldsSelectors.push_back( probFieldSelector );
                }
                m_simGridSelector->onSelection( m_simGridSelector->getCurrentIndex() );
            } else {
                Application::instance()->logWarn("MCRFSimDialog::onRemakeProbabilityFieldsCombos(): failure to retrive the categorical definition for the selected primary variable.");
            }
        } else {
            Application::instance()->logWarn("MCRFSimDialog::onRemakeProbabilityFieldsCombos(): selected attribute is nullptr.");
        }
    }
}

void MCRFSimDialog::onPrimaryVariableChanged()
{
    onRemakeProbabilityFieldsCombos();
    m_commonSimulationParameters->setBaseNameForRealizationVariables( m_primVarSelector->getSelectedVariableName() + "_real" );
}

void MCRFSimDialog::onCommonSimParams()
{
    GSLibParametersDialog gpd( m_commonSimulationParameters, this );
    gpd.setWindowTitle( "Common simulation parameters for MCRF" );
    gpd.exec();
}

void MCRFSimDialog::onRun()
{
    //------------------------------------------------ Build a MCRFSim object ----------------------------------------------------------------
    MCRFSim markovSim( MCRFMode::NORMAL );
    markovSim.m_atPrimary                      = m_primVarSelector->getSelectedVariable();
    markovSim.m_gradationFieldOfPrimaryData    = m_primGradationValueSelector->getSelectedVariable();
    markovSim.m_cgSim                          = dynamic_cast<CartesianGrid*>( m_simGridSelector->getSelectedDataFile() );
    markovSim.m_pdf                            = dynamic_cast<CategoryPDF*>( m_globalPDFSelector->getSelectedFile() );
    markovSim.m_transiogramModel               = dynamic_cast<VerticalTransiogramModel*>( m_verticalTransiogramSelector->getSelectedFile() );
    markovSim.m_gradationFieldOfSimGrid                 = m_gradationalFieldVarSelector->getSelectedVariable();
    for( VariableSelector* probFieldSelector : m_probFieldsSelectors )
        markovSim.m_probFields.push_back( probFieldSelector->getSelectedVariable() );
    markovSim.m_tauFactorForTransiography      = ui->dblSpinTauTransiography->value();
    markovSim.m_tauFactorForProbabilityFields  = ui->dblSpinTauSecondary->value();
    markovSim.m_commonSimulationParameters     = m_commonSimulationParameters;
    markovSim.m_invertGradationFieldConvention = ui->chkInvertGradationFieldConvention->isChecked();
    markovSim.m_maxNumberOfThreads             = ui->spinNumberOfThreads->value();
    //----------------------------------------------------------------------------------------------------------------------------------------

    if( ! markovSim.run() ){
        QMessageBox::critical( this, "Error", QString("Simulation failed.  Check the messages panel for more details of the error."));
        Application::instance()->logError( "MCRFSimDialog::onRun(): Simulation ended with error: ");
        Application::instance()->logError( "    Last error:" + markovSim.getLastError() );
    }
}

