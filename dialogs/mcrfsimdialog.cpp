#include "mcrfsimdialog.h"
#include "ui_mcrfsimdialog.h"

#include <QDragEnterEvent>
#include <QMimeData>

#include "domain/application.h"
#include "domain/projectcomponent.h"
#include "domain/attribute.h"
#include "domain/project.h"
#include "domain/categorydefinition.h"
#include "domain/datafile.h"
#include "widgets/fileselectorwidget.h"
#include "widgets/variableselector.h"
#include "widgets/cartesiangridselector.h"

MCRFSimDialog::MCRFSimDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MCRFSimDialog)
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle( "Markov Chains Random Field Simulation" );

    m_primFileSelector = new FileSelectorWidget( FileSelectorType::PointAndSegmentSets );
    ui->frmCmbPrimFile->layout()->addWidget( m_primFileSelector );

    m_primVarSelector = new VariableSelector( false, VariableSelectorType::CATEGORICAL );
    ui->frmCmbPrimVar->layout()->addWidget( m_primVarSelector );
    connect( m_primFileSelector, SIGNAL(dataFileSelected(DataFile*)),
             m_primVarSelector,  SLOT(onListVariables(DataFile*)) );
    connect( m_primVarSelector,  SIGNAL(currentIndexChanged(int)),
             this,               SLOT(onPrimaryVariableChanged()) );

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
    onCmbLateralGradationChanged();

    //calling this slot causes the sec. variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_simGridSelector->onSelection( 0 );

}

MCRFSimDialog::~MCRFSimDialog()
{
    delete ui;
}

void MCRFSimDialog::onRemakeProbabilityFieldsCombos()
{
    //removes the current comboboxes
    for( VariableSelector* vs : m_probFieldsSelectors )
       delete vs;
    m_probFieldsSelectors.clear();

    //create new ones for each category
    DataFile* df = static_cast<DataFile*>( m_primFileSelector->getSelectedFile() );
    if( df ){
        Attribute* at = m_primVarSelector->getSelectedVariable();
        if( at ){
            CategoryDefinition* cd = df->getCategoryDefinition( at );
            if( cd ){
                for( uint iCatIndex = 0; iCatIndex < cd->getCategoryCount(); ++iCatIndex ){
                    VariableSelector* probFieldSelector = new VariableSelector( );
                    ui->grpBoxSecondaryData->layout()->addWidget( probFieldSelector );
                    connect( m_simGridSelector, SIGNAL(cartesianGridSelected(DataFile*)),
                             probFieldSelector, SLOT(onListVariables(DataFile*)) );
                    m_probFieldsSelectors.push_back( probFieldSelector );
                }
            } else {
                Application::instance()->logError("MCRFSimDialog::onRemakeProbabilityFieldsCombos(): failure to retrive the categorical definition for the selected primary variable.");
            }
        } else {
            Application::instance()->logError("MCRFSimDialog::onRemakeProbabilityFieldsCombos(): selected attribute is nullptr.");
        }
    }
}

void MCRFSimDialog::onCmbLateralGradationChanged()
{
    m_gradationalFieldVarSelector->setEnabled( ui->cmbLateralGradation->currentIndex() == 3 );
}

void MCRFSimDialog::onPrimaryVariableChanged()
{
    onRemakeProbabilityFieldsCombos();
}

