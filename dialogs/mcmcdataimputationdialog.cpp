#include "mcmcdataimputationdialog.h"
#include "ui_mcmcdataimputationdialog.h"

#include "widgets/fileselectorwidget.h"
#include "widgets/variableselector.h"
#include "widgets/univariatedistributionselector.h"
#include "domain/datafile.h"
#include "domain/categorydefinition.h"
#include "domain/application.h"
#include "domain/segmentset.h"
#include "domain/attribute.h"
#include "geostats/mcmcdataimputation.h"

#include <QMessageBox>

MCMCDataImputationDialog::MCMCDataImputationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MCMCDataImputationDialog)
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle( "Data imputation with Markov Chains-Monte Carlo simulation" );

    m_fileSelector = new FileSelectorWidget( FileSelectorType::SegmentSets );
    ui->frmCmbDataSet->layout()->addWidget( m_fileSelector );

    m_varSelector = new VariableSelector( false, VariableSelectorType::CATEGORICAL );
    ui->frmCmbVariable->layout()->addWidget( m_varSelector );
    connect( m_fileSelector, SIGNAL(dataFileSelected(DataFile*)),
             m_varSelector,  SLOT(onListVariables(DataFile*)) );
    connect( m_varSelector,  SIGNAL(currentIndexChanged(int)),
             this,           SLOT(onVariableChanged()) );

    //calling this slot causes the variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_fileSelector->onSelection( 0 );

    m_ftmSelector = new FileSelectorWidget( FileSelectorType::FaciesTransitionMatrices );
    ui->frmCmbFTM->layout()->addWidget( m_ftmSelector );

    onRemakeDistributionCombos();
}



MCMCDataImputationDialog::~MCMCDataImputationDialog()
{
    delete ui;
}

void MCMCDataImputationDialog::onRunMCMC()
{
    //------------------------------------------------ Build a MCMCDataImputation object ----------------------------------------------------------------
    MCMCDataImputation mcmcSim;
    mcmcSim.m_FTM = dynamic_cast<FaciesTransitionMatrix*>( m_ftmSelector->getSelectedFile() );
    mcmcSim.m_seed = ui->spinSeed->value();
    if( ui->cmbFTMtype->currentIndex() == 0 )
        mcmcSim.m_FTMtype = FTMType::FREQUENCIES;
    else
        mcmcSim.m_FTMtype = FTMType::PROBABILITIES;
    mcmcSim.m_dataSet = dynamic_cast<SegmentSet*>( m_fileSelector->getSelectedFile() );
    mcmcSim.m_atVariable = dynamic_cast<Attribute*>( m_varSelector->getSelectedVariable() );
    for( UnivariateDistributionSelector* probFieldSelector : m_distributionSelectors )
        mcmcSim.m_distributions.push_back( probFieldSelector->getSelectedDistribution() );
    //----------------------------------------------------------------------------------------------------------------------------------------

    if( ! mcmcSim.run() ){
        QMessageBox::critical( this, "Error", QString("Simulation failed.  Check the messages panel for more details of the error."));
        Application::instance()->logError( "MCMCDataImputationDialog::onRun(): Simulation ended with error: ");
        Application::instance()->logError( "    Last error:" + mcmcSim.getLastError() );
    } else {
        // TODO: add save to data set code here.
    }
}

void MCMCDataImputationDialog::onVariableChanged()
{
    onRemakeDistributionCombos();
}

void MCMCDataImputationDialog::onRemakeDistributionCombos()
{
    //removes the current comboboxes
    for( UnivariateDistributionSelector* uds : m_distributionSelectors )
       delete uds; //this pointer is managed by Qt, so this causes its detachement from the dialog's UI.
    m_distributionSelectors.clear();

    //create new ones for each category
    DataFile* df = static_cast<DataFile*>( m_fileSelector->getSelectedFile() );
    if( df ){
        Attribute* at = m_varSelector->getSelectedVariable();
        if( at ){
            CategoryDefinition* cd = df->getCategoryDefinition( at );
            if( cd ){
                cd->loadQuintuplets(); //loads the C.D. data from the filesystem.
                for( uint iCatIndex = 0; iCatIndex < cd->getCategoryCount(); ++iCatIndex ){
                    UnivariateDistributionSelector* probFieldSelector = new UnivariateDistributionSelector( );
                    probFieldSelector->setCaption( "   " + cd->getCategoryName( iCatIndex ) + "   " );
                    probFieldSelector->setCaptionBGColor( cd->getCustomColor( iCatIndex ) );
                    ui->frmDistributions->layout()->addWidget( probFieldSelector );
                    m_distributionSelectors.push_back( probFieldSelector );
                }
            } else {
                Application::instance()->logWarn("MCMCDataImputationDialog::onRemakeDistributionCombos(): failure to retrive the categorical definition for the selected primary variable.");
            }
        } else {
            Application::instance()->logWarn("MCMCDataImputationDialog::onRemakeDistributionCombos(): selected attribute is nullptr.");
        }
    }

}
