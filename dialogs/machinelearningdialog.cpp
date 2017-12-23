#include "machinelearningdialog.h"
#include "ui_machinelearningdialog.h"
#include "widgets/fileselectorwidget.h"
#include "widgets/variableselector.h"
#include "domain/attribute.h"
#include "domain/datafile.h"
#include "domain/application.h"
#include "algorithms/CART/cart.h"
#include "algorithms/ialgorithmdatasource.h"
#include "algorithms/randomforest.h"

#include <QInputDialog>

#include <limits>

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
    QString prefix = "<html><head/><body><p><span style=\" font-weight:600; color:#0000ff;\">";
    QString suffix = "</span></p></body></html>";
    if( isClassification() )
        ui->lblApplicationType->setText( prefix + "Classification" + suffix);
    else
        ui->lblApplicationType->setText( prefix + "Regression" + suffix);
}

void MachineLearningDialog::runAlgorithm()
{
    if( ui->cmbAlgorithmType->currentText() == "CART" ){
        if( isClassification() )
            runCARTClassify();
        else
            runCARTRegression();
    }else if( ui->cmbAlgorithmType->currentText() == "Random Forest" ){
        if( isClassification() )
            runRandomForestClassify();
        else
            Application::instance()->logError("MachineLearningDialog::runAlgorithm(): regression not supported yet with Random Forest.");
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

void MachineLearningDialog::runCARTClassify()
{
    //suggest a new attribute name to the user
    QString proposed_name( m_trainingDependentVariableSelector->getSelectedVariableName() );
    proposed_name = proposed_name.append( "_CLASSIFIED_WITH_CART" );

    //presents a dialog so the user can change the suggested name for the new categorical variable.
    bool ok;
    QString new_var_name = QInputDialog::getText(this, "Name the class variable",
                                             "New variable name:", QLineEdit::Normal,
                                             proposed_name, &ok);

    //if the user cancelled the input box, do nothing.
    if (! ok )
        return;

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
    Application::instance()->logInfo("MachineLearningDialog::runCARTClassify(): CART tree built.");

    //for each output data
    long outputRowCount = outputDataFile->getDataLineCount();
    std::vector<double> classValues;  //vector to hold the results (must be double for the final GEO-EAS file)
    classValues.reserve( outputRowCount );
    for( long outputRow = 0; outputRow < outputRowCount; ++outputRow){
        //classify the data
        std::list< std::pair< DataValue, long> > result;
        CARTalgorithm.classify( outputRow,
                                m_trainingDependentVariableSelector->getSelectedVariableGEOEASIndex()-1,
                                result);
        //get the results
        std::list< std::pair< DataValue, long> >::iterator it = result.begin();
        for( ; it != result.end(); ++it ){
            classValues.push_back( (*it).first.getCategorical() );
            break; //TODO: this causes only the first class value to be considerd
                   //      other values may come with different counts (assign uncertainty)
        }
    }

    //Get the Attribute object corresponding to the dependent variable (categorical)
    Attribute* at = trainingDataFile->getAttributeFromGEOEASIndex(
                    m_trainingDependentVariableSelector->getSelectedVariableGEOEASIndex() );

    //add the results as a categorical attribute
    outputDataFile->addNewDataColumn( new_var_name, classValues, trainingDataFile->getCategoryDefinition(at) );
}

void MachineLearningDialog::runCARTRegression()
{
    //suggest a new attribute name to the user
    QString proposed_name( m_trainingDependentVariableSelector->getSelectedVariableName() );
    proposed_name = proposed_name.append( "_ESTIMATED_WITH_CART" );

    //presents a dialog so the user can change the suggested name for the new continuous variable.
    bool ok;
    QString new_var_name = QInputDialog::getText(this, "Name the variable",
                                             "New variable name:", QLineEdit::Normal,
                                             proposed_name, &ok);

    //if the user cancelled the input box, do nothing.
    if (! ok )
        return;

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
    Application::instance()->logInfo("MachineLearningDialog::runCARTRegression(): CART tree built.");

    //for each output data
    long outputRowCount = outputDataFile->getDataLineCount();
    std::vector<double> estimatedValues;  //vector to hold the estimations
    std::vector<double> percentages;  //vector to hold the percentages
    estimatedValues.reserve( outputRowCount );
    percentages.reserve( outputRowCount );
    for( long outputRow = 0; outputRow < outputRowCount; ++outputRow){
        //regress the data
        DataValue mean( 0.0d );
        double percent;
        CARTalgorithm.regress(  outputRow,
                                m_trainingDependentVariableSelector->getSelectedVariableGEOEASIndex()-1,
                                mean,
                                percent);
        //get the results
        estimatedValues.push_back( mean.getContinuous() );
        percentages.push_back( percent );
    }

    //add the results as continuous attributes
    outputDataFile->addNewDataColumn( new_var_name, estimatedValues );
    outputDataFile->addNewDataColumn( new_var_name + "_percent", percentages );
}

void MachineLearningDialog::runRandomForestClassify()
{
    bool ok;

    //ask the user to enter the number of trees
    int B = QInputDialog::getInt( this, "User input", "Number of trees: ", 1, 1, 5000, 1, &ok);
    if( ! ok )
        return;

    //ask the user to enter the seed for the random number generator
    int seed = QInputDialog::getInt( this, "User input", "Seed for the random number generator: ",
                                     69069, 1, std::numeric_limits<int>::max(), 1, &ok);
    if( ! ok )
        return;

    //suggest a new attribute name to the user
    QString proposed_name( m_trainingDependentVariableSelector->getSelectedVariableName() );
    proposed_name = proposed_name.append( "_CLASSIFIED_WITH_RF" );

    //presents a dialog so the user can change the suggested name for the new categorical variable.
    QString new_var_name = QInputDialog::getText(this, "Name the class variable",
                                             "New variable name:", QLineEdit::Normal,
                                             proposed_name, &ok);

    //if the user cancelled the input box, do nothing.
    if (! ok )
        return;

    //Get the selected data files.
    DataFile* trainingDataFile = (DataFile*)m_trainingFileSelector->getSelectedFile();
    DataFile* outputDataFile = (DataFile*)m_outputFileSelector->getSelectedFile();

    //load the data from filesystem.
    trainingDataFile->loadData();
    outputDataFile->loadData();

    //get the data source interface for the algorithms.
    std::list<int> trainingFeaturesIDList = getTrainingFeaturesIDList();
    std::list<int> outputFeaturesIDList = getOutputFeaturesIDList();

    //Build the RandomForest object, containing the Random Forest algorithm.
    RandomForest RF( *trainingDataFile->algorithmDataSource(),
                     *outputDataFile->algorithmDataSource(),
                     trainingFeaturesIDList,
                     outputFeaturesIDList,
                     B, //number of trees
                     seed ); //seed for the random number generator
    Application::instance()->logInfo("MachineLearningDialog::runRandomForestClassify(): RandomForest object built.");

    //get the number of data rows in the output to be classified
    long outputRowCount = outputDataFile->getDataLineCount();

    //create an array to hold the results for each decision tree
    //double -> GEO-EAS data are stored as doubles
    std::vector<double> classes( outputRowCount, std::numeric_limits<double>::quiet_NaN() );
    std::vector<double> counts( outputRowCount, std::numeric_limits<double>::quiet_NaN() );

    //for each output data
    for( long outputRow = 0; outputRow < outputRowCount; ++outputRow){
        //classify the data
        //First value is the class and the second is uncertainty
        std::pair< DataValue, double> result( DataValue((int)0), 1.0 );
        RF.classify( outputRow,
                     m_trainingDependentVariableSelector->getSelectedVariableGEOEASIndex()-1,
                     result );
        //get the result
        classes[outputRow] = result.first.getCategorical();
        counts[outputRow] = result.second;
    }

    Application::instance()->logInfo("MachineLearningDialog::runRandomForestClassify(): classification completed.");

    //Get the Attribute object corresponding to the dependent variable (categorical)
    Attribute* at = trainingDataFile->getAttributeFromGEOEASIndex(
                    m_trainingDependentVariableSelector->getSelectedVariableGEOEASIndex() );

    //add the classification as a categorical attribute
    outputDataFile->addNewDataColumn( new_var_name, classes, trainingDataFile->getCategoryDefinition(at) );

    //add the counts as a common variable
    outputDataFile->addNewDataColumn( new_var_name + "_uncert", counts );

    Application::instance()->logInfo("MachineLearningDialog::runRandomForestClassify(): finished.");
}

bool MachineLearningDialog::isClassification()
{
    DataFile* trainingFile = (DataFile*)m_trainingFileSelector->getSelectedFile();
    if( trainingFile ){
        Attribute* at = trainingFile->getAttributeFromGEOEASIndex(
                    m_trainingDependentVariableSelector->getSelectedVariableGEOEASIndex() );
        if( at )
            return at->isCategorical();
    }
    return false; //default case is continuous (regression application).
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
