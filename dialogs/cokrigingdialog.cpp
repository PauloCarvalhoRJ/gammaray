#include "cokrigingdialog.h"
#include "ui_cokrigingdialog.h"
#include "domain/application.h"
#include "widgets/pointsetselector.h"
#include "widgets/variableselector.h"
#include "widgets/cartesiangridselector.h"
#include "widgets/variogrammodelselector.h"
#include "util.h"

CokrigingDialog::CokrigingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CokrigingDialog)
{

    ui->setupUi(this);

    this->setWindowTitle("Cokriging");

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    //The list with existing point sets in the project for the input data.
    m_psInputSelector = new PointSetSelector();
    ui->frmData->layout()->addWidget( m_psInputSelector );
    QFont font = m_psInputSelector->font();
    font.setBold( false );
    m_psInputSelector->setFont( font );

    //The list for primary variable selection.
    m_inputPrimVarSelector = makeVariableSelector();
    ui->frmData->layout()->addWidget( m_inputPrimVarSelector );
    m_inputPrimVarSelector->setFont( font );
    connect( m_psInputSelector, SIGNAL(pointSetSelected(DataFile*)),
             m_inputPrimVarSelector, SLOT(onListVariables(DataFile*)) );
    connect( m_inputPrimVarSelector, SIGNAL(variableSelected(Attribute*)),
             this, SLOT(onUpdateVarMatrixLabels()));

    //The list with existing cartesian grids in the project for the estimation.
    m_cgEstimationGridSelector = new CartesianGridSelector();
    ui->frmGrid->layout()->addWidget( m_cgEstimationGridSelector );
    font = m_cgEstimationGridSelector->font();
    font.setBold( false );
    m_cgEstimationGridSelector->setFont( font );

    //The list with existing cartesian grids in the project for the secondary data.
    m_cgSecondaryGridSelector = new CartesianGridSelector( true );
    ui->frmSecondaryData->layout()->addWidget( m_cgSecondaryGridSelector );
    font = m_cgSecondaryGridSelector->font();
    font.setBold( false );
    m_cgSecondaryGridSelector->setFont( font );

    //Call this slot to create the widgets that are function of the number of secondary variables
    onNumberOfSecondaryVariablesChanged( 1 );

    //calling this slot causes the variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_psInputSelector->onSelection( 0 );
    m_cgSecondaryGridSelector->onSelection( 0 );
}

CokrigingDialog::~CokrigingDialog()
{
    delete ui;
    Application::instance()->logInfo("CokrigingDialog destroyed.");
}

void CokrigingDialog::onNumberOfSecondaryVariablesChanged(int n)
{
    //clears the current set of secondary variable selectors
    QVector<VariableSelector*>::iterator it = m_inputSecVarsSelectors.begin();
    for(; it != m_inputSecVarsSelectors.end(); ++it){
        delete (*it);
    }
    m_inputSecVarsSelectors.clear();

    //installs the target number of secondary variable selectors
    for( int i = 0; i < n; ++i){
        VariableSelector* selector = makeVariableSelector();
        ui->frmData->layout()->addWidget( selector );
        connect( m_psInputSelector, SIGNAL(pointSetSelected(DataFile*)),
                 selector, SLOT(onListVariables(DataFile*)) );
        connect( selector, SIGNAL(variableSelected(Attribute*)),
                 this, SLOT(onUpdateVarMatrixLabels()));
        m_inputSecVarsSelectors.push_back( selector );
    }
    //--------------------------------------------------------------------

    //clears the current set of secondary grid variable selectors
    it = m_inputGridSecVarsSelectors.begin();
    for(; it != m_inputGridSecVarsSelectors.end(); ++it){
        delete (*it);
    }
    m_inputGridSecVarsSelectors.clear();

    //installs the target number of secondary grid variable selectors
    for( int i = 0; i < n; ++i){
        VariableSelector* selector = makeVariableSelector();
        ui->frmSecondaryData->layout()->addWidget( selector );
        connect( m_cgSecondaryGridSelector, SIGNAL(cartesianGridSelected(DataFile*)),
                 selector, SLOT(onListVariables(DataFile*)) );
        m_inputGridSecVarsSelectors.push_back( selector );
    }
    //---------------------------------------------------------------------

    //clears the current set of lebels in the variography matrix top bar
    QVector<QLabel*>::iterator itLabels = m_labelsVarMatrixTopHeader.begin();
    for(; itLabels != m_labelsVarMatrixTopHeader.end(); ++itLabels){
        delete (*itLabels);
    }
    m_labelsVarMatrixTopHeader.clear();
    //---------------------------------------------------------------------

    //clears the current set of lebels in the variography matrix left bar
    itLabels = m_labelsVarMatrixLeftHeader.begin();
    for(; itLabels != m_labelsVarMatrixLeftHeader.end(); ++itLabels){
        delete (*itLabels);
    }
    m_labelsVarMatrixLeftHeader.clear();
    //---------------------------------------------------------------------

    //calling this slot causes the variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_psInputSelector->onSelection( 0 );
    m_cgSecondaryGridSelector->onSelection( 0 );

    //update the table of variogram widgets
    onUpdateVariogramMatrix( n );
}

void CokrigingDialog::onUpdateVariogramMatrix( int numberOfSecondaryVariables )
{
    //get the pointer to the grid layout
    QGridLayout* layout = (QGridLayout*)ui->frmVariogramArray->layout();

    //clean the current widgets
    while (QLayoutItem* item = layout->takeAt( 0 ))
       delete item->widget();

    //clean the lists of created widgets
    m_labelsVarMatrixTopHeader.clear();
    m_labelsVarMatrixLeftHeader.clear();

    //place holder at row=0, col=0
    layout->addWidget( new QLabel(), 0, 0, 1, 1 );

    //label for the primary variable in the table top header
    m_labelsVarMatrixTopHeader.append( makeLabel( m_inputPrimVarSelector->getSelectedVariableName() ) );
    layout->addWidget( m_labelsVarMatrixTopHeader.last() , 0, 2, 1, 1 );
    //label for the first secondary variable in the table top header
    m_labelsVarMatrixTopHeader.append( makeLabel( m_inputSecVarsSelectors[0]->getSelectedVariableName() ) );
    layout->addWidget(  m_labelsVarMatrixTopHeader.last(), 0, 3, 1, 1 );

    //separator under the table header
    layout->addWidget( Util::createHorizontalLine(), 1, 0, 1, 3 + numberOfSecondaryVariables );

    //label for the primary variable in the table left header
    m_labelsVarMatrixLeftHeader.append( makeLabel( m_inputPrimVarSelector->getSelectedVariableName() ) );
    layout->addWidget( m_labelsVarMatrixLeftHeader.last(), 2, 0, 1, 1 );
    //label for the first secondary variable in the table left header
    m_labelsVarMatrixLeftHeader.append( makeLabel( m_inputSecVarsSelectors[0]->getSelectedVariableName() ) );
    layout->addWidget( m_labelsVarMatrixLeftHeader.last(), 3, 0, 1, 1 );

    //separator after the table left header
    layout->addWidget( Util::createVerticalLine(), 0, 1, 3 + numberOfSecondaryVariables, 1 );

    //labels for possibly further secondary variables
    for( int i = 1; i < numberOfSecondaryVariables; ++i){
        m_labelsVarMatrixTopHeader.append( makeLabel( m_inputSecVarsSelectors[i]->getSelectedVariableName() ) );
        layout->addWidget( m_labelsVarMatrixTopHeader.last(), 0, 3+i, 1, 1 );
         m_labelsVarMatrixLeftHeader.append( makeLabel( m_inputSecVarsSelectors[i]->getSelectedVariableName() ) );
        layout->addWidget( m_labelsVarMatrixLeftHeader.last() , 3+i, 0, 1, 1 );
    }

    //The list with existing variogram models for the primary variable.
    VariogramModelSelector* vModelSelector = makeVariogramModelSelector();
    layout->addWidget( vModelSelector, 2, 2, 1, 1 );

    //The list with existing variogram models for the 1st secondary variable.
    vModelSelector = makeVariogramModelSelector();
    layout->addWidget( vModelSelector, 3, 3, 1, 1 );

    //The list with existing variogram models for the primary/1st secondary cross variography.
    vModelSelector = makeVariogramModelSelector();
    layout->addWidget( vModelSelector, 2, 3, 1, 1 );

    //Lists of variogram models for possibly further secondary variables
    for( int i = 1; i < numberOfSecondaryVariables; ++i){
        for( int j = 0; j < i + 2; ++j){
            vModelSelector = makeVariogramModelSelector();
            layout->addWidget( vModelSelector, 2+j, 3+i, 1, 1 );
        }
    }
}

void CokrigingDialog::onUpdateVarMatrixLabels()
{
    //must have at least one primary and one secondary variable
    if( m_labelsVarMatrixTopHeader.size() < 2 )
        return;

    QVector<QLabel*>::iterator itLabels = m_labelsVarMatrixTopHeader.begin();
    QVector<QLabel*>::iterator itLeftLabels = m_labelsVarMatrixLeftHeader.begin();

    //first label is that for the primary variable
    (*itLabels)->setText( m_inputPrimVarSelector->getSelectedVariableName() );
    (*itLeftLabels)->setText( m_inputPrimVarSelector->getSelectedVariableName() );
    ++itLabels;
    ++itLeftLabels;

    //update the labels for the secondary variables
    QVector<VariableSelector*>::iterator itSelectors = m_inputSecVarsSelectors.begin();
    for(; itSelectors != m_inputSecVarsSelectors.end(); ++itLabels, ++itLeftLabels, ++itSelectors){
        (*itLabels)->setText( (*itSelectors)->getSelectedVariableName() );
        (*itLeftLabels)->setText( (*itSelectors)->getSelectedVariableName() );
    }
}

void CokrigingDialog::onParameters()
{

}

QLabel *CokrigingDialog::makeLabel(const QString caption)
{
    QLabel* label = new QLabel( caption );
    label->setStyleSheet("font-weight: normal;");
    label->setAlignment( Qt::AlignHCenter );
    return label;
}

VariableSelector *CokrigingDialog::makeVariableSelector()
{
    VariableSelector* vs = new VariableSelector( );
    vs->setStyleSheet("font-weight: normal;");
    return vs;
}

VariogramModelSelector *CokrigingDialog::makeVariogramModelSelector()
{
    VariogramModelSelector* vs = new VariogramModelSelector( );
    vs->setStyleSheet("font-weight: normal;");
    return vs;
}
