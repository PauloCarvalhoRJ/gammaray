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
    m_inputPrimVarSelector = new VariableSelector( );
    ui->frmData->layout()->addWidget( m_inputPrimVarSelector );
    font = m_inputPrimVarSelector->font();
    font.setBold( false );
    m_inputPrimVarSelector->setFont( font );
    connect( m_psInputSelector, SIGNAL(pointSetSelected(DataFile*)),
             m_inputPrimVarSelector, SLOT(onListVariables(DataFile*)) );

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

    onUpdateVariogramMatrix( 1 );
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
        VariableSelector* selector = new VariableSelector( );
        ui->frmData->layout()->addWidget( selector );
        QFont font = selector->font();
        font.setBold( false );
        selector->setFont( font );
        connect( m_psInputSelector, SIGNAL(pointSetSelected(DataFile*)),
                 selector, SLOT(onListVariables(DataFile*)) );
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
        VariableSelector* selector = new VariableSelector( );
        ui->frmSecondaryData->layout()->addWidget( selector );
        QFont font = selector->font();
        font.setBold( false );
        selector->setFont( font );
        connect( m_cgSecondaryGridSelector, SIGNAL(cartesianGridSelected(DataFile*)),
                 selector, SLOT(onListVariables(DataFile*)) );
        m_inputGridSecVarsSelectors.push_back( selector );
    }
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

    //place holder at row=0, col=0
    layout->addWidget( new QLabel(), 0, 0, 1, 1 );

    //label for the primary variable in the table top header
    layout->addWidget( new QLabel( m_inputPrimVarSelector->getSelectedVariableName() ), 0, 2, 1, 1 );
    //label for the first secondary variable in the table top header
    layout->addWidget( new QLabel( m_inputSecVarsSelectors[0]->getSelectedVariableName() ), 0, 3, 1, 1 );

    //separator under the table header
    layout->addWidget( Util::createHorizontalLine(), 1, 0, 1, 3 + numberOfSecondaryVariables );

    //label for the primary variable in the table left header
    layout->addWidget( new QLabel( m_inputPrimVarSelector->getSelectedVariableName() ), 2, 0, 1, 1 );
    //label for the first secondary variable in the table left header
    layout->addWidget( new QLabel( m_inputSecVarsSelectors[0]->getSelectedVariableName() ), 3, 0, 1, 1 );

    //separator after the table left header
    layout->addWidget( Util::createVerticalLine(), 0, 1, 3 + numberOfSecondaryVariables, 1 );

    //labels for possibly further secondary variables
    for( int i = 1; i < numberOfSecondaryVariables; ++i){
        layout->addWidget( new QLabel( m_inputSecVarsSelectors[i]->getSelectedVariableName() ), 0, 3+i, 1, 1 );
        layout->addWidget( new QLabel( m_inputSecVarsSelectors[i]->getSelectedVariableName() ), 3+i, 0, 1, 1 );
    }

    //The list with existing variogram models for the primary variable.
    VariogramModelSelector* vModelSelector = new VariogramModelSelector();
    layout->addWidget( vModelSelector, 2, 2, 1, 1 );

    //The list with existing variogram models for the 1st secondary variable.
    vModelSelector = new VariogramModelSelector();
    layout->addWidget( vModelSelector, 3, 3, 1, 1 );

    //The list with existing variogram models for the primary/1st secondary cross variography.
    vModelSelector = new VariogramModelSelector();
    layout->addWidget( vModelSelector, 2, 3, 1, 1 );

    //Lists of variogram models for possibly further secondary variables
    for( int i = 1; i < numberOfSecondaryVariables; ++i){
        for( int j = 0; j < i + 2; ++j){
            vModelSelector = new VariogramModelSelector();
            layout->addWidget( vModelSelector, 2+j, 3+i, 1, 1 );
        }
    }
}
