#include "sisimdialog.h"
#include "ui_sisimdialog.h"
#include "widgets/pointsetselector.h"
#include "widgets/variableselector.h"
#include "widgets/fileselectorwidget.h"
#include "widgets/cartesiangridselector.h"
#include "widgets/variogrammodelselector.h"
#include "domain/file.h"
#include "domain/pointset.h"
#include "domain/categorypdf.h"
#include "domain/thresholdcdf.h"
#include "domain/application.h"
#include "domain/project.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslibparams/widgets/gslibparamwidgets.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"

#include <QMessageBox>

SisimDialog::SisimDialog(IKVariableType varType, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SisimDialog),
    m_varType( varType ),
    m_gpf_sisim( nullptr )
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
    bool firstRun = false;

    //---------------------- Initial checks and data to configure simulation-----------------------

    //get the selected p.d.f./c.d.f. file
    File *distribution = m_DensityFunctionSelector->getSelectedFile();
    if( ! distribution ){
        QMessageBox::critical( this, "Error", "Please, select a c.d.f./p.d.f. file.");
        return;
    }

    //get the selected point set data file
    PointSet *inputPointSet = (PointSet*)m_InputPointSetFileSelector->getSelectedDataFile();
    if( ! inputPointSet ){
        QMessageBox::critical( this, "Error", "Please, select an input point set file.");
        return;
    }

    //get min and max of variable
    double data_min = inputPointSet->min( m_InputVariableSelector->getSelectedVariableGEOEASIndex()-1 );
    double data_max = inputPointSet->max( m_InputVariableSelector->getSelectedVariableGEOEASIndex()-1 );
    data_min -= fabs( data_min/100.0 );
    data_max += fabs( data_max/100.0 );

    //ValuePairs class implements getContentsCount() to return the number of pairs,
    //which is the number of thresholds/categories.
    uint nThresholdsOrCategories = distribution->getContentsCount();

    //The GEO-EAS index (first == 1)
    int varIndex = m_InputVariableSelector->getSelectedVariableGEOEASIndex();

    //-----------------------------set sisim parameters---------------------------
    if( ! m_gpf_sisim ){
        //create the parameters object
        m_gpf_sisim = new GSLibParameterFile("sisim");

        //set the default values, so we need to set fewer parameters here
        m_gpf_sisim->setDefaultValues();

        firstRun = true;
    }

    //SISIM mode: 1=continuous(cdf), 0=categorical(pdf)
    if( m_varType == IKVariableType::CONTINUOUS )
        m_gpf_sisim->getParameter<GSLibParOption*>(0)->_selected_value = 1;
    else if( m_varType == IKVariableType::CATEGORICAL )
        m_gpf_sisim->getParameter<GSLibParOption*>(0)->_selected_value = 0;

    //number thresholds/categories
    m_gpf_sisim->getParameter<GSLibParUInt*>(1)->_value = nThresholdsOrCategories;

    //   thresholds / categories, which is a <double+>
    GSLibParMultiValuedVariable *par2 = m_gpf_sisim->getParameter<GSLibParMultiValuedVariable*>(2);
    par2->setSize( nThresholdsOrCategories );
    for( uint i = 0; i < nThresholdsOrCategories; ++i )
        if( m_varType == IKVariableType::CATEGORICAL ){
            CategoryPDF *pdf = (CategoryPDF*)distribution;
            par2->getParameter<GSLibParDouble*>(i)->_value = pdf->get1stValue(i);
        } else {
            ThresholdCDF *cdf = (ThresholdCDF*)distribution;
            par2->getParameter<GSLibParDouble*>(i)->_value = cdf->get1stValue(i);
        }

    //   global cdf / pdf, which is a <double+>
    GSLibParMultiValuedVariable *par3 = m_gpf_sisim->getParameter<GSLibParMultiValuedVariable*>(3);
    par3->setSize( nThresholdsOrCategories );
    for( uint i = 0; i < nThresholdsOrCategories; ++i )
        if( m_varType == IKVariableType::CATEGORICAL ){
            CategoryPDF *pdf = (CategoryPDF*)distribution;
            par3->getParameter<GSLibParDouble*>(i)->_value = pdf->get2ndValue(i);
        } else {
            ThresholdCDF *cdf = (ThresholdCDF*)distribution;
            par3->getParameter<GSLibParDouble*>(i)->_value = cdf->get2ndValue(i);
        }

    //file with data
    m_gpf_sisim->getParameter<GSLibParFile*>(4)->_path = inputPointSet->getPath();

    //   columns for X,Y,Z, and variable
    GSLibParMultiValuedFixed *par5 = m_gpf_sisim->getParameter<GSLibParMultiValuedFixed*>(5);
    par5->getParameter<GSLibParUInt*>(0)->_value = inputPointSet->getXindex();
    par5->getParameter<GSLibParUInt*>(1)->_value = inputPointSet->getYindex();
    par5->getParameter<GSLibParUInt*>(2)->_value = inputPointSet->getZindex();
    par5->getParameter<GSLibParUInt*>(3)->_value = varIndex;

    //the soft indicator file is surely a PointSet object
    PointSet *psSoftData = (PointSet*)m_SoftPointSetSelector->getSelectedDataFile();
    if( psSoftData ){
        //file with soft indicator input
        m_gpf_sisim->getParameter<GSLibParFile*>(6)->_path = psSoftData->getPath();
        //columns for X,Y,Z, and indicators
        GSLibParMultiValuedFixed *par7 = m_gpf_sisim->getParameter<GSLibParMultiValuedFixed*>(7);
        par7->getParameter<GSLibParUInt*>(0)->_value = psSoftData->getXindex();
        par7->getParameter<GSLibParUInt*>(1)->_value = psSoftData->getYindex();
        par7->getParameter<GSLibParUInt*>(2)->_value = psSoftData->getZindex();
        GSLibParMultiValuedVariable* par7_3 = par7->getParameter<GSLibParMultiValuedVariable*>(3);
        par7_3->setSize( nThresholdsOrCategories );
        for( uint i = 0; i < nThresholdsOrCategories; ++i ){ // the indicator column indexes, which are a <uint+>
            par7_3->getParameter<GSLibParUInt*>(i)->_value =
                    m_SoftIndicatorVariablesSelectors[i]->getSelectedVariableGEOEASIndex();
        }
        //      calibration B(z) values, which is a <double+>
        GSLibParMultiValuedVariable *par9 = m_gpf_sisim->getParameter<GSLibParMultiValuedVariable*>(9);
        par9->setSize( nThresholdsOrCategories );
    }

    //trimming limits
    if( firstRun ){
        GSLibParMultiValuedFixed *par10 = m_gpf_sisim->getParameter<GSLibParMultiValuedFixed*>(10);
        par10->getParameter<GSLibParDouble*>(0)->_value = data_min;
        par10->getParameter<GSLibParDouble*>(1)->_value = data_max;
    }

    //minimum (zmin) and maximum (zmax) data value
    if( firstRun ){
        GSLibParMultiValuedFixed *par11 = m_gpf_sisim->getParameter<GSLibParMultiValuedFixed*>(11);
        par11->getParameter<GSLibParDouble*>(0)->_value = data_min;
        par11->getParameter<GSLibParDouble*>(1)->_value = data_max;
    }

    //file for simulation output
    if( firstRun )
        m_gpf_sisim->getParameter<GSLibParFile*>(19)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");

    //grid parameters
    m_gridParametersWidget->updateValue( m_parGrid );
    GSLibParGrid* par21 = m_gpf_sisim->getParameter<GSLibParGrid*>(21);
    par21->_specs_x->getParameter<GSLibParUInt*>(0)->_value = m_parGrid->getNX(); //nx
    par21->_specs_x->getParameter<GSLibParDouble*>(1)->_value = m_parGrid->getX0(); //min x
    par21->_specs_x->getParameter<GSLibParDouble*>(2)->_value = m_parGrid->getDX(); //cell size x
    par21->_specs_y->getParameter<GSLibParUInt*>(0)->_value = m_parGrid->getNY(); //ny
    par21->_specs_y->getParameter<GSLibParDouble*>(1)->_value = m_parGrid->getY0(); //min y
    par21->_specs_y->getParameter<GSLibParDouble*>(2)->_value = m_parGrid->getDY(); //cell size y
    par21->_specs_z->getParameter<GSLibParUInt*>(0)->_value = m_parGrid->getNZ(); //nz
    par21->_specs_z->getParameter<GSLibParDouble*>(1)->_value = m_parGrid->getZ0(); //min z
    par21->_specs_z->getParameter<GSLibParDouble*>(2)->_value = m_parGrid->getDZ(); //cell size z

    //IK mode: 0=full, 1=median  and cutoff for the median mode
    GSLibParMultiValuedFixed *par32 = m_gpf_sisim->getParameter<GSLibParMultiValuedFixed*>(32);
    if( ui->radioFullIK->isChecked() )
        par32->getParameter<GSLibParOption*>(0)->_selected_value = 0;
    else
        par32->getParameter<GSLibParOption*>(0)->_selected_value = 1;

    //Variogram models for each threshold/category of one variogram model if IK is in median mode.
    GSLibParRepeat *par34 = m_gpf_sisim->getParameter<GSLibParRepeat*>(34);
    if( ui->radioMedianIK->isChecked() ){
        par34->setCount( 1 );
        GSLibParVModel *par34_0 = par34->getParameter<GSLibParVModel*>(0, 0);
        VariogramModelSelector* vms = m_variogramSelectors.at( 0 );
        VariogramModel *vmodel = vms->getSelectedVModel();
        par34_0->setFromVariogramModel( vmodel );
    } else {
        par34->setCount( nThresholdsOrCategories );
        for( uint i = 0; i < nThresholdsOrCategories; ++i){
            GSLibParVModel *par34_0 = par34->getParameter<GSLibParVModel*>(i, 0);
            VariogramModelSelector* vms = m_variogramSelectors.at( i );
            VariogramModel *vmodel = vms->getSelectedVModel();
            par34_0->setFromVariogramModel( vmodel );
        }
    }
}
