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
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "gslib/gslibparametersdialog.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"

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
            runRandomForestRegression();
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

bool MachineLearningDialog::getCARTParameters(GSLibParameterFile &gpf)
{
    GSLibParInt* par0 = new GSLibParInt( "MaxSplitsContinuous", "", "Max splits for continuous features:" );
    par0->_value = 20;
    gpf.addParameter( par0 );
    //suggest a new attribute name to the user
    QString proposed_name( m_trainingDependentVariableSelector->getSelectedVariableName() );
    if( isClassification() )
        proposed_name = proposed_name.append( "_CLASSIFIED_WITH_CART" );
    else
        proposed_name = proposed_name.append( "_ESTIMATED_WITH_CART" );
    GSLibParString* par4 = new GSLibParString("VariableName", "", "New variable name: ");
    par4->_value = proposed_name;
    gpf.addParameter( par4 );

    //Use the GSLib parameter dialog to allow the user to edit the Random Forest parameters
    GSLibParametersDialog diag( &gpf, this );
    diag.setWindowTitle("Parameters for the CART algorithm");
    int userResponse = diag.exec();
    return userResponse == QDialog::Accepted;
}

void MachineLearningDialog::runCARTClassify()
{
    //use a GSLib parameter file object to hold a collection of parameters for the CART algorithm
    GSLibParameterFile gpf;

    //if the user canceled the parameter dialog, aborts.
    if( ! getCARTParameters(gpf) )
        return;

    //get the seed for the random number generator
    int maxSplitsContinuous = gpf.getParameterByName<GSLibParInt*>("MaxSplitsContinuous")->_value;

    //Get the selected data files.
    DataFile* trainingDataFile = dynamic_cast<DataFile*>(m_trainingFileSelector->getSelectedFile());
    DataFile* outputDataFile = dynamic_cast<DataFile*>(m_outputFileSelector->getSelectedFile());

    //load the data from filesystem.
    trainingDataFile->loadData();
    outputDataFile->loadData();

    //get the data source interface for the algorithms.
    std::vector<int> trainingFeaturesIDList = getTrainingFeaturesIDList();
    std::vector<int> outputFeaturesIDList = getOutputFeaturesIDList();

    //Build the CART tree
    CART CARTalgorithm( *trainingDataFile->algorithmDataSource(),
                        *outputDataFile->algorithmDataSource(),
                        trainingFeaturesIDList,
                        outputFeaturesIDList,
                        maxSplitsContinuous );

    Application::instance()->logInfo("MachineLearningDialog::runCARTClassify(): CART tree built.");

    //for each output data
    long outputRowCount = outputDataFile->getDataLineCount();
    std::vector<double> classValues;  //vector to hold the results (must be double for the final GEO-EAS file)
    classValues.reserve( outputRowCount );
    for( long outputRow = 0; outputRow < outputRowCount; ++outputRow){
        //classify the data
        std::vector< std::pair< DataValue, long> > result;
        CARTalgorithm.classify( outputRow,
                                m_trainingDependentVariableSelector->getSelectedVariableGEOEASIndex()-1,
                                result);
        //get the results
        std::vector< std::pair< DataValue, long> >::iterator it = result.begin();
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
    outputDataFile->addNewDataColumn( gpf.getParameterByName<GSLibParString*>("VariableName")->_value,
                                      classValues,
                                      trainingDataFile->getCategoryDefinition(at) );
}

void MachineLearningDialog::runCARTRegression()
{
    //use a GSLib parameter file object to hold a collection of parameters for the CART algorithm
    GSLibParameterFile gpf;

    //if the user canceled the parameter dialog, aborts.
    if( ! getCARTParameters(gpf) )
        return;

    //get the seed for the random number generator
    int maxSplitsContinuous = gpf.getParameterByName<GSLibParInt*>("MaxSplitsContinuous")->_value;

    //Get the selected data files.
    DataFile* trainingDataFile = dynamic_cast<DataFile*>( m_trainingFileSelector->getSelectedFile() );
    DataFile* outputDataFile = dynamic_cast<DataFile*>( m_outputFileSelector->getSelectedFile() );

    //load the data from filesystem.
    trainingDataFile->loadData();
    outputDataFile->loadData();

    //get the data source interface for the algorithms.
    std::vector<int> trainingFeaturesIDList = getTrainingFeaturesIDList();
    std::vector<int> outputFeaturesIDList = getOutputFeaturesIDList();

    //Build the CART tree
    CART CARTalgorithm( *trainingDataFile->algorithmDataSource(),
                        *outputDataFile->algorithmDataSource(),
                        trainingFeaturesIDList,
                        outputFeaturesIDList,
                        maxSplitsContinuous );

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
    outputDataFile->addNewDataColumn( gpf.getParameterByName<GSLibParString*>("VariableName")->_value, estimatedValues );
    outputDataFile->addNewDataColumn( gpf.getParameterByName<GSLibParString*>("VariableName")->_value + "_percent", percentages );
}

bool MachineLearningDialog::getRandomForestParameters(GSLibParameterFile &gpf )
{
    GSLibParInt* par0 = new GSLibParInt( "Seed", "", "Seed for the random number generator:" );
    par0->_value = 69069;
    gpf.addParameter( par0 );
    GSLibParOption* par1 = new GSLibParOption("Bootstrap", "", "Bootstrap:");
    par1->addOption(1, "Case");
    gpf.addParameter( par1 );
    GSLibParInt *par2 = new GSLibParInt( "B", "", "Number of trees:" );
    par2->_value = 1;
    gpf.addParameter( par2 );
    GSLibParOption* par3 = new GSLibParOption("TreeType", "", "Tree type:");
    par3->addOption(1, "CART");
    gpf.addParameter( par3 );
    GSLibParInt* par5 = new GSLibParInt( "MaxSplitsContinuous", "", "Max decision tree splits for continuous features:" );
    par5->_value = 20;
    gpf.addParameter( par5 );
    //suggest a new attribute name to the user
    QString proposed_name( m_trainingDependentVariableSelector->getSelectedVariableName() );
    if( isClassification() )
        proposed_name = proposed_name.append( "_CLASSIFIED_WITH_RF" );
    else
        proposed_name = proposed_name.append( "_ESTIMATED_WITH_RF" );
    GSLibParString* par4 = new GSLibParString("VariableName", "", "New variable name: ");
    par4->_value = proposed_name;
    gpf.addParameter( par4 );

    //Use the GSLib parameter dialog to allow the user to edit the Random Forest parameters
    GSLibParametersDialog diag( &gpf, this );
    diag.setWindowTitle("Parameters for the Random Forest algorithm");
    int userResponse = diag.exec();
    return userResponse == QDialog::Accepted;
}

void MachineLearningDialog::runRandomForestClassify()
{
    //use a GSLib parameter file object to hold a collection of parameters for the Random Forest algorithm
    GSLibParameterFile gpf;

    //if the user canceled the parameter dialog, aborts.
    if( ! getRandomForestParameters(gpf) )
        return;

    //get the number of trees
    int B = gpf.getParameterByName<GSLibParInt*>("B")->_value;

    //get the seed for the random number generator
    int seed = gpf.getParameterByName<GSLibParInt*>("Seed")->_value;

    //get the bootstrap option
    int bootstrapOption = gpf.getParameterByName<GSLibParOption*>("Bootstrap")->_selected_value;
    ResamplingType bootstrap;
    switch( bootstrapOption ){
    case 1: bootstrap = ResamplingType::CASE; break;
    default:
        Application::instance()->logError("MachineLearningDialog::runRandomForestClassify(): unrecognized bootstrap option: "
                                          + QString::number( bootstrapOption ) + ". Aborted.");
        return;
    }

    //get the tree type option
    int treeTypeOption = gpf.getParameterByName<GSLibParOption*>("TreeType")->_selected_value;
    TreeType treeType;
    switch( treeTypeOption ){
    case 1: treeType = TreeType::CART; break;
    default:
        Application::instance()->logError("MachineLearningDialog::runRandomForestClassify(): unrecognized tree type option: "
                                          + QString::number( treeTypeOption ) + ". Aborted.");
        return;
    }

    //Get the selected data files.
    DataFile* trainingDataFile = dynamic_cast<DataFile*>( m_trainingFileSelector->getSelectedFile() );
    DataFile* outputDataFile = dynamic_cast<DataFile*>( m_outputFileSelector->getSelectedFile() );

    //load the data from filesystem.
    trainingDataFile->loadData();
    outputDataFile->loadData();

    //get the data source interface for the algorithms.
    std::vector<int> trainingFeaturesIDList = getTrainingFeaturesIDList();
    std::vector<int> outputFeaturesIDList = getOutputFeaturesIDList();

    //Build the RandomForest object, containing the Random Forest algorithm.
    RandomForest RF( *trainingDataFile->algorithmDataSource(),
                     *outputDataFile->algorithmDataSource(),
                     trainingFeaturesIDList,
                     outputFeaturesIDList,
                     B, //number of trees
                     seed, //seed for the random number generator
                     bootstrap, //how the training data is re-sampled in each RF iteration
                     treeType, //the type of trees used to build the forest
                     gpf.getParameterByName<GSLibParInt*>("MaxSplitsContinuous")->_value);
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
    outputDataFile->addNewDataColumn( gpf.getParameterByName<GSLibParString*>("VariableName")->_value,
                                      classes, trainingDataFile->getCategoryDefinition(at) );

    //add the counts as a common variable
    outputDataFile->addNewDataColumn( gpf.getParameterByName<GSLibParString*>("VariableName")->_value + "_uncert", counts );

    Application::instance()->logInfo("MachineLearningDialog::runRandomForestClassify(): finished.");
}

void MachineLearningDialog::runRandomForestRegression()
{
    //use a GSLib parameter file object to hold a collection of parameters for the Random Forest algorithm
    GSLibParameterFile gpf;

    //if the user canceled the parameter dialog, aborts.
    if( ! getRandomForestParameters(gpf) )
        return;

    //get the number of trees
    int B = gpf.getParameterByName<GSLibParInt*>("B")->_value;

    //get the seed for the random number generator
    int seed = gpf.getParameterByName<GSLibParInt*>("Seed")->_value;

    //get the bootstrap option
    int bootstrapOption = gpf.getParameterByName<GSLibParOption*>("Bootstrap")->_selected_value;
    ResamplingType bootstrap;
    switch( bootstrapOption ){
    case 1: bootstrap = ResamplingType::CASE; break;
    default:
        Application::instance()->logError("MachineLearningDialog::runRandomForestRegression(): unrecognized bootstrap option: "
                                          + QString::number( bootstrapOption ) + ". Aborted.");
        return;
    }

    //get the tree type option
    int treeTypeOption = gpf.getParameterByName<GSLibParOption*>("TreeType")->_selected_value;
    TreeType treeType;
    switch( treeTypeOption ){
    case 1: treeType = TreeType::CART; break;
    default:
        Application::instance()->logError("MachineLearningDialog::runRandomForestRegression(): unrecognized tree type option: "
                                          + QString::number( treeTypeOption ) + ". Aborted.");
        return;
    }

    //Get the selected data files.
    DataFile* trainingDataFile = dynamic_cast<DataFile*>(m_trainingFileSelector->getSelectedFile());
    DataFile* outputDataFile = dynamic_cast<DataFile*>(m_outputFileSelector->getSelectedFile());

    //load the data from filesystem.
    trainingDataFile->loadData();
    outputDataFile->loadData();

    //get the data source interface for the algorithms.
    std::vector<int> trainingFeaturesIDList = getTrainingFeaturesIDList();
    std::vector<int> outputFeaturesIDList = getOutputFeaturesIDList();

    //Build the RandomForest object, containing the Random Forest algorithm.
    RandomForest RF( *trainingDataFile->algorithmDataSource(),
                     *outputDataFile->algorithmDataSource(),
                     trainingFeaturesIDList,
                     outputFeaturesIDList,
                     B, //number of trees
                     seed, //seed for the random number generator
                     bootstrap, //how the training data is re-sampled in each RF iteration
                     treeType, //the type of trees used to build the forest
                     gpf.getParameterByName<GSLibParInt*>("MaxSplitsContinuous")->_value);
    Application::instance()->logInfo("MachineLearningDialog::runRandomForestRegression(): RandomForest object built.");

    //get the number of data rows in the output to be estimated
    long outputRowCount = outputDataFile->getDataLineCount();

    //create vectors to hold the results for each decision tree
    //double -> GEO-EAS data are stored as doubles
    std::vector<double> means( outputRowCount, std::numeric_limits<double>::quiet_NaN() );
    std::vector<double> variances( outputRowCount, std::numeric_limits<double>::quiet_NaN() );

    //for each output data
    for( long outputRow = 0; outputRow < outputRowCount; ++outputRow){
        //estimate the data
        //First value is the class and the second is uncertainty
        DataValue mean( 0.0d );
        DataValue variance( 0.0d );
        RF.regress( outputRow,
                    m_trainingDependentVariableSelector->getSelectedVariableGEOEASIndex()-1,
                    mean,
                    variance );
        //get the result
        means[outputRow] = mean.getContinuous();
        variances[outputRow] = variance.getContinuous();
    }

    Application::instance()->logInfo("MachineLearningDialog::runRandomForestRegression(): regression completed.");

    //add the regression and its variance as continuous attributes
    outputDataFile->addNewDataColumn( gpf.getParameterByName<GSLibParString*>("VariableName")->_value, means );
    outputDataFile->addNewDataColumn( gpf.getParameterByName<GSLibParString*>("VariableName")->_value + "_variance", variances );

    Application::instance()->logInfo("MachineLearningDialog::runRandomForestRegression(): finished.");
}

bool MachineLearningDialog::isClassification()
{
    DataFile* trainingFile = dynamic_cast<DataFile*>(m_trainingFileSelector->getSelectedFile());
    if( trainingFile ){
        Attribute* at = trainingFile->getAttributeFromGEOEASIndex(
                    m_trainingDependentVariableSelector->getSelectedVariableGEOEASIndex() );
        if( at )
            return at->isCategorical();
    }
    return false; //default case is continuous (regression application).
}

std::vector<int> MachineLearningDialog::getTrainingFeaturesIDList()
{
    std::vector<int> result;
    QVector<VariableSelector*>::iterator it = m_trainingVariableSelectors.begin();
    for( ; it != m_trainingVariableSelectors.end(); ++it )
        result.push_back( (*it)->getSelectedVariableGEOEASIndex()-1 );
    return result;
}

std::vector<int> MachineLearningDialog::getOutputFeaturesIDList()
{
    std::vector<int> result;
    QVector<VariableSelector*>::iterator it = m_outputVariableSelectors.begin();
    for( ; it != m_outputVariableSelectors.end(); ++it )
        result.push_back( (*it)->getSelectedVariableGEOEASIndex()-1 );
    return result;
}
