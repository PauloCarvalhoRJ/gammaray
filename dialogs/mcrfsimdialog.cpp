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
#include "gslib/gslibparametersdialog.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"

MCRFSimDialog::MCRFSimDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MCRFSimDialog),
    m_primFileSelector( nullptr ),
    m_primVarSelector( nullptr ),
    m_simGridSelector( nullptr ),
    m_verticalTransiogramSelector( nullptr ),
    m_globalPDFSelector( nullptr ),
    m_gradationalFieldVarSelector( nullptr ),
    m_LVAazVarSelector( nullptr ),
    m_LVAsemiMajorAxisVarSelector( nullptr ),
    m_LVAsemiMinorAxisVarSelector( nullptr )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle( "Markov Chains Random Field Simulation" );

    connect( ui->chkUseProbabilityFields, SIGNAL(clicked()), this, SLOT(onPrimaryVariableChanged()));

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

    m_LVAazVarSelector = new VariableSelector( );
    m_LVAsemiMajorAxisVarSelector = new VariableSelector( );
    m_LVAsemiMinorAxisVarSelector = new VariableSelector( );
    ui->frmCmbLVAazSelector->layout()->addWidget( m_LVAazVarSelector );
    ui->frmCmbLVAmajorSelector->layout()->addWidget( m_LVAsemiMajorAxisVarSelector );
    ui->frmCmbLVAminorSelector->layout()->addWidget( m_LVAsemiMinorAxisVarSelector );
    connect( m_simGridSelector,             SIGNAL(cartesianGridSelected(DataFile*)),
             m_LVAazVarSelector,              SLOT(onListVariables(DataFile*)) );
    connect( m_simGridSelector,             SIGNAL(cartesianGridSelected(DataFile*)),
             m_LVAsemiMajorAxisVarSelector,   SLOT(onListVariables(DataFile*)) );
    connect( m_simGridSelector,             SIGNAL(cartesianGridSelected(DataFile*)),
             m_LVAsemiMinorAxisVarSelector,   SLOT(onListVariables(DataFile*)) );

    //calling this slot causes the sec. variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_simGridSelector->onSelection( 0 );

    onRemakeProbabilityFieldsCombos();
}

MCRFSimDialog::~MCRFSimDialog()
{
    delete ui;
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

void MCRFSimDialog::onCmbLateralGradationChanged()
{
    m_gradationalFieldVarSelector->setEnabled( ui->cmbLateralGradation->currentIndex() == 3 );
}

void MCRFSimDialog::onPrimaryVariableChanged()
{
    onRemakeProbabilityFieldsCombos();
}

void MCRFSimDialog::onCommonSimParams()
{
    std::vector<QString> linesOfTemplateSyntax = {
        "<uint>                                                -Number of realizations",                       // 0
        "<uint>                                                -Seed for random number generator",             // 1
        "<uint><uint>                                          -Min. and max. primary data for conditioning",  // 2
        "<uint>                                                -Number of simulated nodes for conditioning",   // 3
        "<option [0:no] [1:yes]>                               -Assign data to nodes",                         // 4
        "<option [0:no] [1:yes]><uint>                         -Use multigrid search (0=no, 1=yes), number",   // 5
        "<uint>                                                -Max. data per octant (0=not used)",            // 6
        "<double><double><double>                              -Search ellipsoid: radii (hmax,hmin,vert)",     // 7
        "<double><double><double>                              -Search ellipsoid: angles"                      // 8
    };
    GSLibParameterFile gpf( linesOfTemplateSyntax );
    gpf.getParameter<GSLibParUInt*>(0)->_value = 1;
    gpf.getParameter<GSLibParUInt*>(1)->_value = 69069;
    GSLibParMultiValuedFixed* par2 = gpf.getParameter<GSLibParMultiValuedFixed*>(2);{
        par2->getParameter<GSLibParUInt*>(0)->_value = 4;
        par2->getParameter<GSLibParUInt*>(1)->_value = 8;
    }
    gpf.getParameter<GSLibParUInt*>(3)->_value = 16;
    gpf.getParameter<GSLibParOption*>(4)->_selected_value = 0;
    GSLibParMultiValuedFixed* par5 = gpf.getParameter<GSLibParMultiValuedFixed*>(5);{
        par5->getParameter<GSLibParOption*>(0)->_selected_value = 0;
        par5->getParameter<GSLibParUInt*>(1)->_value = 3;
    }
    gpf.getParameter<GSLibParUInt*>(6)->_value = 0;
    GSLibParMultiValuedFixed* par7 = gpf.getParameter<GSLibParMultiValuedFixed*>(7);{
        par7->getParameter<GSLibParDouble*>(0)->_value = 10.0;
        par7->getParameter<GSLibParDouble*>(1)->_value = 10.0;
        par7->getParameter<GSLibParDouble*>(2)->_value = 1.0;
    }
    GSLibParMultiValuedFixed* par8 = gpf.getParameter<GSLibParMultiValuedFixed*>(8);{
        par8->getParameter<GSLibParDouble*>(0)->_value = 0.0;
        par8->getParameter<GSLibParDouble*>(1)->_value = 0.0;
        par8->getParameter<GSLibParDouble*>(2)->_value = 0.0;
    }


    GSLibParametersDialog gpd( &gpf, this );
    gpd.setWindowTitle( "Common simulation parameters for MCRF" );
    gpd.exec();
}

