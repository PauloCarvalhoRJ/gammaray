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
#include "domain/categorypdf.h"
#include "domain/project.h"
#include "geostats/mcmcdataimputation.h"

#include <QInputDialog>
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

    m_groupByVariableSelector = new VariableSelector( true );
    ui->frmCmbGroupByVariable->layout()->addWidget( m_groupByVariableSelector );
    connect( m_fileSelector, SIGNAL(dataFileSelected(DataFile*)),
             m_groupByVariableSelector,  SLOT(onListVariables(DataFile*)) );

    m_PDFSelector = new FileSelectorWidget( FileSelectorType::PDFs, true );
    ui->frmCmbPDF->layout()->addWidget( m_PDFSelector );


    //calling this slot causes the variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_fileSelector->onSelection( 0 );

    m_ftmSelector = new FileSelectorWidget( FileSelectorType::FaciesTransitionMatrices );
    ui->frmCmbFTM->layout()->addWidget( m_ftmSelector );

    m_enforceFtmSelector = new FileSelectorWidget( FileSelectorType::FaciesTransitionMatrices, true );
    ui->frmCmbEnforceFTM->layout()->addWidget( m_enforceFtmSelector );

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
    //----- the pairs category code/distribution -------
    DataFile* df = static_cast<DataFile*>( m_fileSelector->getSelectedFile() );
    assert( df && "MCMCDataImputationDialog::onRunMCMC(): null DataFile pointer.");
    Attribute* at = m_varSelector->getSelectedVariable();
    assert( at && "MCMCDataImputationDialog::onRunMCMC(): null Attribute pointer.");
    CategoryDefinition* cd = df->getCategoryDefinition( at );
    assert( cd && "MCMCDataImputationDialog::onRunMCMC(): null CategoryDefinition pointer.");
    cd->loadQuintuplets(); //loads the data from the filesystem.
    for( uint iCatIndex = 0; iCatIndex < cd->getCategoryCount(); ++iCatIndex ){
        mcmcSim.m_distributions.insert( { cd->getCategoryCode( iCatIndex ),
                                          m_distributionSelectors[ iCatIndex ]->getSelectedDistribution() } );
    }
    //--------------------------------------------------
    mcmcSim.m_pdfForImputationWithPreviousUnavailable = dynamic_cast<CategoryPDF*>( m_PDFSelector->getSelectedFile() );
    mcmcSim.m_sequenceDirection = static_cast<SequenceDirection>( ui->cmbOrder->currentIndex() );
    mcmcSim.m_atVariableGroupBy = dynamic_cast<Attribute*>( m_groupByVariableSelector->getSelectedVariable() );
    mcmcSim.setNumberOfRealizations( ui->spinRealizations->value() );
    mcmcSim.m_enforceFTM = dynamic_cast<FaciesTransitionMatrix*>( m_enforceFtmSelector->getSelectedFile() );
    mcmcSim.m_enforceThreshold = ui->dblSpinEnforceThreshold->value();
    //----------------------------------------------------------------------------------------------------------------------------------------

    Application::instance()->logInfo("Commencing MCMC simulation...");
    if( ! mcmcSim.run() ){
        QMessageBox::critical( this, "Error", QString("Simulation failed.  Check the messages panel for more details of the error."));
        Application::instance()->logError( "MCMCDataImputationDialog::onRun(): Simulation ended with error: ");
        Application::instance()->logError( "    Last error:" + mcmcSim.getLastError() );
    } else {
        Application::instance()->logInfo("MCMC simulation completed.");

        //open the renaming dialog
        bool ok;
        QString suggestedName = mcmcSim.m_dataSet->getName() + "_IMPUTED";
        QString new_file_name = QInputDialog::getText(this, "Name the new segment set file",
                                                 "New file name:", QLineEdit::Normal, suggestedName, &ok);
        if( ! ok )
            return;

        for( uint iReal = 0; iReal < mcmcSim.getNumberOfRealizations(); ++iReal ){

            //make the path for the file.
            QString new_file_path = Application::instance()->getProject()->getPath() + "/" +
                                    new_file_name + ui->txtRealizationBaseName->text() +
                                    QString::number( iReal );

            //Creates a new segment set object to house the imputed data.
            SegmentSet* imputed_ss = new SegmentSet( new_file_path );

            //Set the same metadata of the original data set.
            imputed_ss->setInfoFromAnotherSegmentSet( mcmcSim.m_dataSet );

            //causes a population of child Attribute objects matching the ones from the original imput data set
            imputed_ss->setPath( mcmcSim.m_dataSet->getPath() );
            imputed_ss->updateChildObjectsCollection();

            //loads the original data into the imputed data set
            imputed_ss->loadData();

            //adds a new Attribute corresponding to the imputed=1/0 flag, along with an extra data column
            imputed_ss->addEmptyDataColumn( "imputed", imputed_ss->getDataLineCount() );

            //replaces original data with the imputed data
            imputed_ss->replaceDataFrame( mcmcSim.getImputedDataFrames()[ iReal ] );

            //creates the new physical file
            imputed_ss->setPath( new_file_path );
            imputed_ss->writeToFS();

            //save its metadata file
            imputed_ss->updateMetaDataFile();

            //causes an update to the child objects in the project tree
            imputed_ss->setInfoFromMetadataFile();

            //attach the object to the project tree
            Application::instance()->getProject()->addDataFile( imputed_ss );

            //show the newly created object in main window's project tree
            Application::instance()->refreshProjectTree();
        }
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
