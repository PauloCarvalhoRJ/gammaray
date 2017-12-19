#include "machinelearningdialog.h"
#include "ui_machinelearningdialog.h"
#include "widgets/fileselectorwidget.h"
#include "widgets/variableselector.h"
#include "domain/attribute.h"
#include "domain/datafile.h"
#include "domain/application.h"
#include "algorithms/CART/cart.h"

MachineLearningDialog::MachineLearningDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MachineLearningDialog)
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    //The training data file selector.
    m_trainingFileSelector = new FileSelectorWidget( FileSelectorType::DataFiles );
    ui->frmTrainingFileSelectorPlaceholder->layout()->addWidget( m_trainingFileSelector );

    //The selector of the dependent variable in the training data file.
    m_trainingDependentVariableSelector = new VariableSelector();
    ui->frmTrainingDependentVariableSelectorPlaceholder->layout()->addWidget( m_trainingDependentVariableSelector );
    connect( m_trainingFileSelector, SIGNAL(dataFileSelected(DataFile*)),
             m_trainingDependentVariableSelector, SLOT(onListVariables(DataFile*)) );
    connect( m_trainingDependentVariableSelector, SIGNAL(variableSelected(Attribute*)),
             this, SLOT(updateApplicationLabel()) );

    //The data file targeted for classification or regression.
    m_outputFileSelector = new FileSelectorWidget( FileSelectorType::DataFiles );
    ui->frmOutputFileSelectorPlaceholder->layout()->addWidget( m_outputFileSelector );

    //Update the variable/features selectors according to user input.
    connect( ui->spinNumberOfFeatures, SIGNAL(valueChanged(int)), this, SLOT(setupVariableSelectionWidgets(int)));

    //Configure the variable/features selectors (both training and output).
    setupVariableSelectionWidgets( ui->spinNumberOfFeatures->value() );

    //calling this slot causes the variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_trainingFileSelector->onSelection( 0 );
}

MachineLearningDialog::~MachineLearningDialog()
{
    delete ui;
}

void MachineLearningDialog::setupVariableSelectionWidgets(int numberOfVariables)
{
    //clears the current set of training variable selectors
    QVector<VariableSelector*>::iterator it = m_trainingVariableSelectors.begin();
    for(; it != m_trainingVariableSelectors.end(); ++it){
        delete (*it);
    }
    m_trainingVariableSelectors.clear();

    //installs the target number of training variable selectors
    for( int i = 0; i < numberOfVariables; ++i){
        VariableSelector* selector = makeVariableSelector();
        ui->frmTrainingPredictiveVariableSelectorsPlaceholder->layout()->addWidget( selector );
        m_trainingVariableSelectors.push_back( selector );
        connect( m_trainingFileSelector, SIGNAL(dataFileSelected(DataFile*)),
                 selector, SLOT(onListVariables(DataFile*)) );
    }

    //clears the current set of output variable selectors
    it = m_outputVariableSelectors.begin();
    for(; it != m_outputVariableSelectors.end(); ++it){
        delete (*it);
    }
    m_outputVariableSelectors.clear();

    //installs the target number of output variable selectors
    for( int i = 0; i < numberOfVariables; ++i){
        VariableSelector* selector = makeVariableSelector();
        ui->frmOutputPredictiveVariableSelectorsPlaceholder->layout()->addWidget( selector );
        m_outputVariableSelectors.push_back( selector );
        connect( m_outputFileSelector, SIGNAL(dataFileSelected(DataFile*)),
                 selector, SLOT(onListVariables(DataFile*)) );
    }

    //calling this slot causes the variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_trainingFileSelector->onSelection( 0 );
    m_outputFileSelector->onSelection( 0 );
}

void MachineLearningDialog::updateApplicationLabel()
{
    DataFile* trainingFile = (DataFile*)m_trainingFileSelector->getSelectedFile();
    if( trainingFile ){
        Attribute* at = trainingFile->getAttributeFromGEOEASIndex(
                    m_trainingDependentVariableSelector->getSelectedVariableGEOEASIndex() );
        if( at ){
            QString prefix = "<html><head/><body><p><span style=\" font-weight:600; color:#0000ff;\">";
            QString suffix = "</span></p></body></html>";
            if( at->isCategorical() )
                ui->lblApplicationType->setText( prefix + "Classification" + suffix);
            else
                ui->lblApplicationType->setText( prefix + "Regression" + suffix);
        }
    }
}

void MachineLearningDialog::runAlgorithm()
{
    if( ui->cmbAlgorithmType->currentText() == "CART" ){
        runCART();
    } else {
        Application::instance()->logError("MachineLearningDialog::runAlgorithm(): unsupported algorithm: " + ui->cmbAlgorithmType->currentText());
    }
}

VariableSelector *MachineLearningDialog::makeVariableSelector()
{
    VariableSelector* vs = new VariableSelector( );
    vs->setStyleSheet("font-weight: normal;");
    return vs;
}

void MachineLearningDialog::runCART()
{
    //Get the selected data files.
    DataFile* trainingDataFile = (DataFile*)m_trainingFileSelector->getSelectedFile();
    DataFile* outputDataFile = (DataFile*)m_outputFileSelector->getSelectedFile();

    //load the data from filesystem.
    trainingDataFile->loadData();
    outputDataFile->loadData();

    //get the data source interface for the algorithms.
    std::list<int> trainingFeaturesIDList = getTrainingFeaturesIDList();
    std::list<int> outputFeaturesIDList = getOutputFeaturesIDList();

    //Build the CART tree
    CART CARTalgorithm( *trainingDataFile->algorithmDataSource(),
                        *outputDataFile->algorithmDataSource(),
                        trainingFeaturesIDList,
                        outputFeaturesIDList);
    Application::instance()->logInfo("MachineLearningDialog::runCART(): CART tree built.");

}

std::list<int> MachineLearningDialog::getTrainingFeaturesIDList()
{
    std::list<int> result;
    QVector<VariableSelector*>::iterator it = m_trainingVariableSelectors.begin();
    for( ; it != m_trainingVariableSelectors.end(); ++it )
        result.push_back( (*it)->getSelectedVariableGEOEASIndex()-1 );
    return result;
}

std::list<int> MachineLearningDialog::getOutputFeaturesIDList()
{
    std::list<int> result;
    QVector<VariableSelector*>::iterator it = m_outputVariableSelectors.begin();
    for( ; it != m_outputVariableSelectors.end(); ++it )
        result.push_back( (*it)->getSelectedVariableGEOEASIndex()-1 );
    return result;
}
