#include "sisimdialog.h"
#include "ui_sisimdialog.h"
#include "widgets/pointsetselector.h"
#include "widgets/variableselector.h"
#include "widgets/fileselectorwidget.h"
#include "widgets/cartesiangridselector.h"
#include "widgets/variogrammodelselector.h"
#include "domain/file.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslibparams/widgets/gslibparamwidgets.h"

SisimDialog::SisimDialog(IKVariableType varType, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SisimDialog)
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    //configure UI captions according to IK variable type
    this->setWindowTitle( "Sequential Indicator Simulation for a " );
    if( varType == IKVariableType::CONTINUOUS ){
        this->setWindowTitle( windowTitle() + "continuous variable." );
        ui->lblIKVarType->setText("<html><head/><body><p><span style=\" font-weight:600; color:#0000ff;\">CONTINUOUS</span></p></body></html>");
        ui->lblDistributionFile->setText("Threshold c.d.f. file:");
    } else {
        this->setWindowTitle( windowTitle() + "categorical variable." );
        ui->lblIKVarType->setText("<html><head/><body><p><span style=\" font-weight:600; color:#0000ff;\">CATEGORICAL</span></p></body></html>");
        ui->lblDistributionFile->setText("Category p.d.f. file:");
    }

    //-----------------------------------Input Data UI----------------------------------------

    //The list with existing point sets in the project.
    m_InputPointSetFileSelector = new PointSetSelector();
    ui->frmInput->layout()->addWidget( m_InputPointSetFileSelector );

    //The list with the Point Set variables to set the variable
    m_InputVariableSelector = new VariableSelector();
    ui->frmInput->layout()->addWidget( m_InputVariableSelector );
    connect( m_InputPointSetFileSelector, SIGNAL(pointSetSelected(DataFile*)),
             m_InputVariableSelector, SLOT(onListVariables(DataFile*)) );

    //calling this slot causes the variable combobox to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_InputPointSetFileSelector->onSelection( 0 );
    m_InputPointSetFileSelector->setSizePolicy( QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred );

    //----------------------------------Soft Data UI------------------------------------------

    //The list with existing point sets in the project for the soft indicators.
    m_SoftPointSetSelector = new PointSetSelector( true );
    ui->frmSoftIndicatorFile->layout()->addWidget( m_SoftPointSetSelector );
    connect( m_SoftPointSetSelector, SIGNAL(pointSetSelected(DataFile*)),
             this, SLOT(onUpdateSoftIndicatorVariablesSelectors()) );

    //---------------------------------Distribution Function UI------------------------------------

    //The list with existing c.d.f./p.d.f. files in the project.
    if( varType == IKVariableType::CONTINUOUS )
        m_DensityFunctionSelector = new FileSelectorWidget( FileSelectorType::CDFs );
    else
        m_DensityFunctionSelector = new FileSelectorWidget( FileSelectorType::PDFs );
    ui->frmDistribution->layout()->addWidget( m_DensityFunctionSelector );
    connect( m_DensityFunctionSelector, SIGNAL(fileSelected(File*)),
             this, SLOT(onUpdateVariogramSelectors()) );
    connect( m_DensityFunctionSelector, SIGNAL(fileSelected(File*)),
             this, SLOT(onUpdateSoftIndicatorVariablesSelectors()) );

    //---------------------------- Grid parameters UI-----------------------------------------

    //The grid parameters widget (reusing the one orginially made for the GSLib parameter dialog).
    m_parGrid = new GSLibParGrid("", "", "");
    m_parGrid->_specs_x->getParameter<GSLibParUInt*>(0)->_value = 100; //nx
    m_parGrid->_specs_x->getParameter<GSLibParDouble*>(1)->_value = 0; //min x
    m_parGrid->_specs_x->getParameter<GSLibParDouble*>(2)->_value = 1; //cell size x
    m_parGrid->_specs_y->getParameter<GSLibParUInt*>(0)->_value = 100; //ny
    m_parGrid->_specs_y->getParameter<GSLibParDouble*>(1)->_value = 0; //min y
    m_parGrid->_specs_y->getParameter<GSLibParDouble*>(2)->_value = 1; //cell size y
    m_parGrid->_specs_z->getParameter<GSLibParUInt*>(0)->_value = 1; //nz
    m_parGrid->_specs_z->getParameter<GSLibParDouble*>(1)->_value = 0; //min z
    m_parGrid->_specs_z->getParameter<GSLibParDouble*>(2)->_value = 1; //cell size z
    m_gridParametersWidget = new WidgetGSLibParGrid();
    ui->frmGridGeomPlaceholder->layout()->addWidget( m_gridParametersWidget );
    m_gridParametersWidget->fillFields( m_parGrid );

    //Grid geometry widgets
    m_gridCopySpecsSelector = new CartesianGridSelector( true );
    ui->frmCopyGridSpecsPlaceholder->layout()->addWidget( m_gridCopySpecsSelector );
    connect( m_gridCopySpecsSelector, SIGNAL(cartesianGridSelected(DataFile*)),
             this, SLOT(onGridCopySpectsSelected(DataFile*)));


    //call this slot to show the variogram selector widgets.
    onUpdateVariogramSelectors();

    //call this slot to show the soft indicator variables selectors.
    onUpdateSoftIndicatorVariablesSelectors();

    adjustSize();
}

SisimDialog::~SisimDialog()
{
    delete ui;
}

void SisimDialog::addVariogramSelector()
{
    VariogramModelSelector* vms = new VariogramModelSelector();
    ui->groupVariograms->layout()->addWidget( vms );
    m_variogramSelectors.append( vms );
}

void SisimDialog::onUpdateSoftIndicatorVariablesSelectors()
{
    //clears the current soft indicator variable selectors
    while( ! m_SoftIndicatorVariablesSelectors.isEmpty() ){
        VariableSelector* vs = m_SoftIndicatorVariablesSelectors.takeLast();
        ui->frmSoftIndicatorVariables->layout()->removeWidget( vs );
        vs->setParent( nullptr );
        delete vs;
    }
    //It is necessary to specify one soft indicator variable per c.d.f./p.d.f threshold/category
    //get the c.d.f./p.d.f. value pairs
    File* file = m_DensityFunctionSelector->getSelectedFile();
    if( file ){
        file->readFromFS();
        int tot = file->getContentsCount();
        for( int i = 0; i < tot; ++i){
            VariableSelector* vs = new VariableSelector();
            ui->frmSoftIndicatorVariables->layout()->addWidget( vs );
            vs->onListVariables( m_SoftPointSetSelector->getSelectedDataFile() );
            m_SoftIndicatorVariablesSelectors.append( vs );
        }
    }

}

void SisimDialog::onUpdateVariogramSelectors()
{
    //clears the current variogram model selectors
    while( ! m_variogramSelectors.isEmpty() ){
        VariogramModelSelector* vms = m_variogramSelectors.takeLast();
        ui->groupVariograms->layout()->removeWidget( vms );
        vms->setParent( nullptr );
        delete vms;
    }
    //if median IK was selected, then only one variogram model is necessary
    if( ui->radioMedianIK->isChecked() ){
        addVariogramSelector();
    } else { //otherwise, it is necessary to specify one variogram model per c.d.f./p.d.f threshold/category
        //get the c.d.f./p.d.f. value pairs
        File* file = m_DensityFunctionSelector->getSelectedFile();
        if( file ){
            file->readFromFS();
            int tot = file->getContentsCount();
            for( int i = 0; i < tot; ++i){
                addVariogramSelector();
            }
        }
    }
}

void SisimDialog::onGridCopySpectsSelected(DataFile *grid)
{
    if( ! grid )
        return;
    m_parGrid->setFromCG((CartesianGrid*)grid);
    m_gridParametersWidget->fillFields( m_parGrid );
}

void SisimDialog::onConfigureAndRun()
{

}
