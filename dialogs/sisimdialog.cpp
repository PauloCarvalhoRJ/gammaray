#include "sisimdialog.h"
#include "ui_sisimdialog.h"
#include "widgets/pointsetselector.h"
#include "widgets/variableselector.h"
#include "widgets/fileselectorwidget.h"
#include "widgets/cartesiangridselector.h"
#include "widgets/variogrammodelselector.h"
#include "widgets/distributionfieldselector.h"
#include "dialogs/displayplotdialog.h"
#include "dialogs/variograminputdialog.h"
#include "domain/file.h"
#include "domain/pointset.h"
#include "domain/categorypdf.h"
#include "domain/thresholdcdf.h"
#include "domain/application.h"
#include "domain/project.h"
#include "domain/cartesiangrid.h"
#include "domain/attribute.h"
#include "domain/variogrammodel.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslibparams/widgets/gslibparamwidgets.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "gslib/gslibparametersdialog.h"
#include "gslib/gslib.h"
#include "util.h"

#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>

SisimDialog::SisimDialog(IKVariableType varType, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SisimDialog),
    m_SoftDataSetSelector( nullptr ),
    m_varType( varType ),
    m_gpf_sisim( nullptr ),
    m_cg_simulation( nullptr ),
	m_gpf_gam( nullptr ),
	m_gpf_postsim( nullptr ),
	m_cg_postsim( nullptr ),
	m_DensityFunctionSecondarySelector( nullptr ),
	m_BidistributionSelector( nullptr ),
	m_BidistPrimaryValuesFieldSelector( nullptr ),
	m_BidistSecondaryValuesFieldSelector( nullptr ),
	m_BidistProbabilitiesFieldSelector( nullptr )
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
		ui->lblDistrFileSecondary->setText("Threshold c.d.f. file for secondary:");
    } else {
        this->setWindowTitle( windowTitle() + "categorical variable." );
        ui->lblIKVarType->setText("<html><head/><body><p><span style=\" font-weight:600; color:#0000ff;\">CATEGORICAL</span></p></body></html>");
        ui->lblDistributionFile->setText("Category p.d.f. file:");
		ui->lblDistrFileSecondary->setText("Category p.d.f. file for secondary:");
		ui->cmbProgram->removeItem(1); // sisim_gs does not make sense for categorical variables
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

    //----------------------------------Soft Data UI------------------------------------------

    configureSoftDataUI();

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

void SisimDialog::preview()
{
    if( m_cg_simulation )
        delete m_cg_simulation;

    QString sisimProgram = ui->cmbProgram->currentText();

    ///////// Set an offset to account for differences in parameter count between the different sisim programs/////////////////////
    int offset = 0;
    if( sisimProgram == "sisim" )
        offset = 0;
    else if ( sisimProgram == "sisim_gs" )
        offset = -1;
	else if ( sisimProgram == "sisim_lm" )
		offset = -3;
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    //get the tmp file path created by sisim with the realizations
    QString grid_file_path = m_gpf_sisim->getParameter<GSLibParFile*>(19 + offset)->_path;

    //create a new grid object corresponding to the file created by sisim
    m_cg_simulation = new CartesianGrid( grid_file_path );

    //set the grid geometry info.
    m_cg_simulation->setInfoFromGridParameter( m_gpf_sisim->getParameter<GSLibParGrid*>(21 + offset) );

    //sisim usually uses -99 as no-data-value. (TODO: this needs to be checked)
    m_cg_simulation->setNoDataValue( "-99" );

    //the number of realizations.
    m_cg_simulation->setNReal( m_gpf_sisim->getParameter<GSLibParUInt*>(20 + offset)->_value );

    if( m_cg_simulation->getChildCount() < 1 ){
		QMessageBox::critical( this, "Error", sisimProgram + " yielded a file without data.  It is possible that " + sisimProgram + " crashed.  Check the message panel.  Aborted.");
        return;
    }

    //get the variable with the simulated values (normally the first)
    Attribute* sim_var = (Attribute*)m_cg_simulation->getChildByIndex( 0 );

    //If the variable type is categorical, display the variable with the colors of the
    //category definition associated with the p.d.f. used to run the simulation.
    //If the category definition is nullptr, then the map will be displayed as
    //a continuous variable.
    CategoryDefinition* cd = nullptr;
    if( m_varType == IKVariableType::CATEGORICAL ){
        //get the selected p.d.f. file
        CategoryPDF *pdf = (CategoryPDF *)m_DensityFunctionSelector->getSelectedFile();
        //get the category definition
        cd = pdf->getCategoryDefinition();
    }

    //open the plot dialog
    Util::viewGrid( sim_var, this, false, cd );

    //enable the action buttons that depend on completion of sisim.
    ui->btnRealizationHistogram->setEnabled( true );
    ui->btnEnsembleHistogram->setEnabled( true );
    ui->btnEnsembleVariogram->setEnabled( true );
    ui->btnSaveRealizations->setEnabled( true );
    ui->btnPostsim->setEnabled( true );

}

void SisimDialog::previewPostsim()
{
	if( m_cg_postsim )
		delete m_cg_postsim;

    QString sisimProgram = ui->cmbProgram->currentText();

    ///////// Set an offset to account for differences in parameter count between the different sisim programs/////////////////////
    int offset = 0;
    if( sisimProgram == "sisim" )
        offset = 0;
    else if ( sisimProgram == "sisim_gs" )
        offset = -1;
	else if ( sisimProgram == "sisim_lm" )
		offset = -3;
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//get the tmp file path created by sgsim with the realizations
	QString grid_file_path = m_gpf_postsim->getParameter<GSLibParFile*>(4)->_path;

	//create a new grid object corresponding to the file created by sgsim
	m_cg_postsim = new CartesianGrid( grid_file_path );

	//set the grid geometry info.
    m_cg_postsim->setInfoFromGridParameter( m_gpf_sisim->getParameter<GSLibParGrid*>(21 + offset) );

	//postsim usually uses -999 as no-data-value.
	m_cg_postsim->setNoDataValue( "-999.0" );

	//Display all variables found in the post-processed grid
	for( uint iVar = 0; iVar < m_cg_postsim->getDataColumnCount(); ++iVar){
		Attribute* est_var = (Attribute*)m_cg_postsim->getChildByIndex( iVar );
		Util::viewGrid( est_var, this );
	}

	//enable the save postsim results button.
    ui->btnSavePostsim->setEnabled( true );
}

void SisimDialog::configureSoftDataUI()
{
    QString sisimProgram = ui->cmbProgram->currentText();

    if( m_SoftDataSetSelector )
        delete m_SoftDataSetSelector;
	if( m_DensityFunctionSecondarySelector )
		delete m_DensityFunctionSecondarySelector;


	//For sisim_gs: the bidistribution file and its field selectors
	{
		if( ! m_BidistributionSelector ){
			m_BidistributionSelector = new FileSelectorWidget( FileSelectorType::Bidistributions ) ;
			ui->frmBidistribution->layout()->addWidget( m_BidistributionSelector );
		}
		if( ! m_BidistPrimaryValuesFieldSelector ){
			m_BidistPrimaryValuesFieldSelector = new DistributionFieldSelector( Roles::DistributionColumnRole::VALUE );
			ui->frmBidistPrimaryField->layout()->addWidget( m_BidistPrimaryValuesFieldSelector );
			connect( m_BidistributionSelector, SIGNAL(distributionSelected(Distribution*)),
					 m_BidistPrimaryValuesFieldSelector, SLOT(onListFields(Distribution*)) );
		}
		if( ! m_BidistSecondaryValuesFieldSelector ){
			m_BidistSecondaryValuesFieldSelector = new DistributionFieldSelector( Roles::DistributionColumnRole::VALUE );
			ui->frmBidistSecondaryField->layout()->addWidget( m_BidistSecondaryValuesFieldSelector );
			connect( m_BidistributionSelector, SIGNAL(distributionSelected(Distribution*)),
					 m_BidistSecondaryValuesFieldSelector, SLOT(onListFields(Distribution*)) );
		}
		if( ! m_BidistProbabilitiesFieldSelector){
			m_BidistProbabilitiesFieldSelector = new DistributionFieldSelector( Roles::DistributionColumnRole::PVALUE );
			ui->frmBidistProbabilities->layout()->addWidget( m_BidistProbabilitiesFieldSelector );
			connect( m_BidistributionSelector, SIGNAL(distributionSelected(Distribution*)),
					 m_BidistProbabilitiesFieldSelector, SLOT(onListFields(Distribution*)) );
		}
		//call this slot to update the field selectors.
		m_BidistributionSelector->onSelection( 0 );
	}

	if( m_varType == IKVariableType::CONTINUOUS )
		m_DensityFunctionSecondarySelector = new FileSelectorWidget( FileSelectorType::CDFs );
	else
		m_DensityFunctionSecondarySelector = new FileSelectorWidget( FileSelectorType::PDFs );
	ui->frmDistrSecondaryData->layout()->addWidget( m_DensityFunctionSecondarySelector );

    //The list with existing data sets in the project for the soft indicators.
    if( sisimProgram == "sisim" ){
        m_SoftDataSetSelector = new FileSelectorWidget( FileSelectorType::PointSets, true );
        ui->frmSoftIndicatorFile->layout()->addWidget( m_SoftDataSetSelector );
        connect( m_SoftDataSetSelector, SIGNAL(dataFileSelected(DataFile*)),
                 this, SLOT(onUpdateSoftIndicatorVariablesSelectors()) );
		ui->lblSecondaryDataFile->setText("Soft indicator file:");
		ui->lblSecondaryDataVariables->setText("Soft indicator variables:");
		ui->lblDistrFileSecondary->hide();
		ui->frmDistrSecondaryData->hide();
		ui->lblBidistribution->hide();
		ui->frmBidistribution->hide();
		ui->frmBidistFields->hide();
	} else if( sisimProgram == "sisim_gs" ){
        //The soft data is mandatory for sisim_gs
        m_SoftDataSetSelector = new FileSelectorWidget( FileSelectorType::CartesianGrids, false );
        ui->frmSoftIndicatorFile->layout()->addWidget( m_SoftDataSetSelector );
        connect( m_SoftDataSetSelector, SIGNAL(dataFileSelected(DataFile*)),
                 this, SLOT(onUpdateSoftIndicatorVariablesSelectors()) );
		ui->lblSecondaryDataFile->setText("Grid with secondary data:");
		ui->lblSecondaryDataVariables->setText("Secondary variable:");
		ui->lblDistrFileSecondary->show();
		ui->frmDistrSecondaryData->show();
		ui->lblBidistribution->show();
		ui->frmBidistribution->show();
		ui->frmBidistFields->show();
	} else if( sisimProgram == "sisim_lm" ){
		//The secondary data is mandatory for sisim_lm
		m_SoftDataSetSelector = new FileSelectorWidget( FileSelectorType::CartesianGrids, false );
		ui->frmSoftIndicatorFile->layout()->addWidget( m_SoftDataSetSelector );
		connect( m_SoftDataSetSelector, SIGNAL(dataFileSelected(DataFile*)),
				 this, SLOT(onUpdateSoftIndicatorVariablesSelectors()) );
		ui->lblSecondaryDataFile->setText("Grid with means (soft indicators):");
		ui->lblSecondaryDataVariables->hide();
		ui->lblDistrFileSecondary->hide();
		ui->frmDistrSecondaryData->hide();
		ui->lblBidistribution->hide();
		ui->frmBidistribution->hide();
		ui->frmBidistFields->hide();
	}

    //call this slot to show the soft indicator variables selectors.
    onUpdateSoftIndicatorVariablesSelectors();
}

void SisimDialog::onUpdateSoftIndicatorVariablesSelectors()
{
	QString sisimProgram = ui->cmbProgram->currentText();

	//clears the current soft indicator variable selectors
    while( ! m_SoftIndicatorVariablesSelectors.isEmpty() ){
        VariableSelector* vs = m_SoftIndicatorVariablesSelectors.takeLast();
        ui->frmSoftIndicatorVariables->layout()->removeWidget( vs );
        vs->setParent( nullptr );
        delete vs;
    }

    //It is necessary to specify one soft indicator variable per c.d.f./p.d.f threshold/category
    //get the c.d.f./p.d.f. value pairs
	if( sisimProgram == "sisim" ){
		File* file = m_DensityFunctionSelector->getSelectedFile();
		if( file ){
			file->readFromFS();
			int tot = file->getContentsCount();
			for( int i = 0; i < tot; ++i){
				VariableSelector* vs = new VariableSelector();
				ui->frmSoftIndicatorVariables->layout()->addWidget( vs );
				vs->onListVariables( static_cast<DataFile*>(m_SoftDataSetSelector->getSelectedFile()) );
				m_SoftIndicatorVariablesSelectors.append( vs );
			}
		}
	} else if ( sisimProgram == "sisim_gs" ){ // sisim_gs requires the secondary variable directly and not converted into soft indicators
		VariableSelector* vs = new VariableSelector();
		ui->frmSoftIndicatorVariables->layout()->addWidget( vs );
		vs->onListVariables( static_cast<DataFile*>(m_SoftDataSetSelector->getSelectedFile()) );
		m_SoftIndicatorVariablesSelectors.append( vs );
	}
	// sisim_lm does not require to set the secondary data variables
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
    //get the name of the SISIM program the user wants.
    QString sisimProgram = ui->cmbProgram->currentText();

    //get the selected p.d.f./c.d.f. file
    File *distribution = m_DensityFunctionSelector->getSelectedFile();
    if( ! distribution ){
        QMessageBox::critical( this, "Error", "Please, select a c.d.f./p.d.f. file.");
        return;
    }

	//get the selected p.d.f./c.d.f. file for the secondary (for sisim_gs)
	File *distributionSecondary = m_DensityFunctionSecondarySelector->getSelectedFile();
	if( sisimProgram == "sisim_gs" && ! distributionSecondary ){
		QMessageBox::critical( this, "Error", "Please, select a c.d.f./p.d.f. file for the secondary variable.");
		return;
	}

    //get the selected point set data file
    PointSet *inputPointSet = (PointSet*)m_InputPointSetFileSelector->getSelectedDataFile();
    if( ! inputPointSet ){
        QMessageBox::critical( this, "Error", "Please, select an input point set file.");
        return;
    }

    //get min and max of variable
    inputPointSet->loadData();
	double data_min = inputPointSet->min( m_InputVariableSelector->getSelectedVariableGEOEASIndex()-1 );
    double data_max = inputPointSet->max( m_InputVariableSelector->getSelectedVariableGEOEASIndex()-1 );
	data_min -= fabs( data_min/100.0 );
    data_max += fabs( data_max/100.0 );

	//get min and max of secondary variable (for sisim_gs)
    double secData_min =  std::numeric_limits<double>::max();
    double secData_max = -std::numeric_limits<double>::max();
	if( sisimProgram == "sisim_gs" ){
		DataFile* secondaryDataFile = static_cast<DataFile*>( m_SoftDataSetSelector->getSelectedFile() );
		if( secondaryDataFile ){
			secData_min = secondaryDataFile->min( m_SoftIndicatorVariablesSelectors[0]->getSelectedVariableGEOEASIndex()-1 );
			secData_max = secondaryDataFile->max( m_SoftIndicatorVariablesSelectors[0]->getSelectedVariableGEOEASIndex()-1 );
			secData_min -= fabs( secData_min/100.0 );
			secData_max += fabs( secData_max/100.0 );
		}
	}

    //ValuePairs class implements getContentsCount() to return the number of pairs,
    //which is the number of thresholds/categories.
    uint nThresholdsOrCategories = distribution->getContentsCount();

    //The GEO-EAS index (first == 1)
    int varIndex = m_InputVariableSelector->getSelectedVariableGEOEASIndex();

	//-----------------------------for sisim_gs it is necessary to run bicalib beforehand-----------
	QString pathOfFileWithCalibrationTable;
	if( sisimProgram == "sisim_gs" ){
		GSLibParameterFile gpfBicalib( "bicalib" );
		gpfBicalib.setDefaultValues();
		//-file with secondary data
		gpfBicalib.getParameter<GSLibParFile*>(0)->_path = m_SoftDataSetSelector->getSelectedFile()->getPath();
		//-   column for secondary variable
		gpfBicalib.getParameter<GSLibParUInt*>(1)->_value = m_SoftIndicatorVariablesSelectors[0]->getSelectedVariableGEOEASIndex(); //for sisim_gs, only the secondary (not its soft indicators) is selectable
		//-file with calibration scatterplot
		gpfBicalib.getParameter<GSLibParFile*>(2)->_path = m_BidistributionSelector->getSelectedFile()->getPath();
		//-   columns of pri, sec, and weight
		GSLibParMultiValuedFixed *par3 = gpfBicalib.getParameter<GSLibParMultiValuedFixed*>(3);
		par3->getParameter<GSLibParUInt*>(0)->_value = m_BidistPrimaryValuesFieldSelector->getSelectedFieldGEOEASIndex();
		par3->getParameter<GSLibParUInt*>(1)->_value = m_BidistSecondaryValuesFieldSelector->getSelectedFieldGEOEASIndex();
		par3->getParameter<GSLibParUInt*>(2)->_value = m_BidistProbabilitiesFieldSelector->getSelectedFieldGEOEASIndex();
		//-   trimming limits
		GSLibParMultiValuedFixed *par4 = gpfBicalib.getParameter<GSLibParMultiValuedFixed*>(4);
		par4->getParameter<GSLibParDouble*>(0)->_value = std::min( data_min, secData_min );
		par4->getParameter<GSLibParDouble*>(1)->_value = std::max( data_max, secData_max );
		//-file for output data / distributions
		gpfBicalib.getParameter<GSLibParFile*>(5)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "dat" );
		//-file for output calibration (SISIM)
		gpfBicalib.getParameter<GSLibParFile*>(6)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "calib" );
		pathOfFileWithCalibrationTable = gpfBicalib.getParameter<GSLibParFile*>(6)->_path;
		//-file for calibration report
		gpfBicalib.getParameter<GSLibParFile*>(7)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "rpt" );
		//-number of thresholds on primary
		gpfBicalib.getParameter<GSLibParUInt*>(8)->_value = nThresholdsOrCategories;
		//-   thresholds on primary
		GSLibParMultiValuedVariable *par9 = gpfBicalib.getParameter<GSLibParMultiValuedVariable*>(9);
		par9->setSize( nThresholdsOrCategories );
		for( uint i = 0; i < nThresholdsOrCategories; ++i )
			if( m_varType == IKVariableType::CATEGORICAL ){
				CategoryPDF *pdf = (CategoryPDF*)distribution;
				par9->getParameter<GSLibParDouble*>(i)->_value = pdf->get1stValue(i);
			} else {
				ThresholdCDF *cdf = (ThresholdCDF*)distribution;
				par9->getParameter<GSLibParDouble*>(i)->_value = cdf->get1stValue(i);
			}
		//-number of thresholds on secondary
		uint nThresholdsOrCategoriesForSecondary = distributionSecondary->getContentsCount();
		gpfBicalib.getParameter<GSLibParUInt*>(10)->_value = nThresholdsOrCategoriesForSecondary;
		//-   thresholds on secondary
		GSLibParMultiValuedVariable *par11 = gpfBicalib.getParameter<GSLibParMultiValuedVariable*>(11);
		par11->setSize( nThresholdsOrCategoriesForSecondary );
		for( uint i = 0; i < nThresholdsOrCategoriesForSecondary; ++i )
			if( m_varType == IKVariableType::CATEGORICAL ){
				CategoryPDF *pdf = (CategoryPDF*)distribution;
				par11->getParameter<GSLibParDouble*>(i)->_value = pdf->get1stValue(i);
			} else {
				ThresholdCDF *cdf = (ThresholdCDF*)distribution;
				par11->getParameter<GSLibParDouble*>(i)->_value = cdf->get1stValue(i);
			}
		//Generate the parameter file
		QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "par" );
		gpfBicalib.save( par_file_path );
		//run bicalib program
		Application::instance()->logInfo("Starting bicalib program...");
		GSLib::instance()->runProgram( "bicalib", par_file_path );
		//verify whether the required output was generated.
		QFile file( pathOfFileWithCalibrationTable );
		if( ! file.exists() ){
			QMessageBox::critical( this, "Error", "Calibration file not found.  Probably bicalib execution failed.  Check the messages panel for bicalib output.");
			return;
		}
	}

	//-----------------------------set sisim parameters-------------------------------------
    if( ! m_gpf_sisim ){
        //create the parameters object
        m_gpf_sisim = new GSLibParameterFile( sisimProgram );

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

    //the soft indicator file
    if( sisimProgram == "sisim" ){
        //for sisim, it is a point set file
        PointSet *psSoftData = static_cast<PointSet*>( m_SoftDataSetSelector->getSelectedFile() );
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
		} else {
			m_gpf_sisim->getParameter<GSLibParFile*>(6)->_path = "NO_DATA_FILE";
		}
    } else if ( sisimProgram == "sisim_gs" ){
		//for sisim_gs, it is a cartesian file (this file is mandatory in sisim_gs)
        CartesianGrid *cgSoftData = static_cast<CartesianGrid*>( m_SoftDataSetSelector->getSelectedFile() );
        if( cgSoftData ){
            //file with soft indicator input
            m_gpf_sisim->getParameter<GSLibParFile*>(6)->_path = cgSoftData->getPath();
			//for sisim_gs, the soft indicator file is a grid file and uses the secondary variable directly
            GSLibParMultiValuedVariable *par7 = m_gpf_sisim->getParameter<GSLibParMultiValuedVariable*>(7);
			par7->setSize( 1 );
			par7->getParameter<GSLibParUInt*>(0)->_value = m_SoftIndicatorVariablesSelectors[0]->getSelectedVariableGEOEASIndex();
        }
	} else if ( sisimProgram == "sisim_lm" ){
		//for sisim_lm, it is a cartesian file (this file is mandatory in sisim_lm)
		CartesianGrid *cgSoftData = static_cast<CartesianGrid*>( m_SoftDataSetSelector->getSelectedFile() );
		if( cgSoftData ){
			//file with soft indicator input
			m_gpf_sisim->getParameter<GSLibParFile*>(6)->_path = cgSoftData->getPath();
			//for sisim_lm, the soft indicator file is a grid file and no secondary variable is specified
		}
	}

    ///////// Set an offset to account for differences in parameter count between the different sisim programs/////////////////////
    int offset = 0;
    if( sisimProgram == "sisim" )
        offset = 0;
    else if ( sisimProgram == "sisim_gs" )
        offset = -1;
	else if ( sisimProgram == "sisim_lm" )
		offset = -3;
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    if( sisimProgram == "sisim"){
        //      calibration B(z) values, which is a <double+> (only for sisim)
        GSLibParMultiValuedVariable *par9 = m_gpf_sisim->getParameter<GSLibParMultiValuedVariable*>(9 + offset);
        par9->setSize( nThresholdsOrCategories );
    } else if( sisimProgram == "sisim_gs" ){
        //file with calibration table (only for sisim_gs)
		m_gpf_sisim->getParameter<GSLibParFile*>(9 + offset)->_path = pathOfFileWithCalibrationTable;
    }

    //trimming limits
    if( firstRun ){
        GSLibParMultiValuedFixed *par10 = m_gpf_sisim->getParameter<GSLibParMultiValuedFixed*>(10 + offset);
		par10->getParameter<GSLibParDouble*>(0)->_value = data_min;
        par10->getParameter<GSLibParDouble*>(1)->_value = data_max;
    }

    //minimum (zmin) and maximum (zmax) data value
    if( firstRun ){
        GSLibParMultiValuedFixed *par11 = m_gpf_sisim->getParameter<GSLibParMultiValuedFixed*>(11 + offset);
		par11->getParameter<GSLibParDouble*>(0)->_value = data_min;
        par11->getParameter<GSLibParDouble*>(1)->_value = data_max;
    }

    //file for simulation output
    if( firstRun )
        m_gpf_sisim->getParameter<GSLibParFile*>(19 + offset)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");

    //grid parameters
    m_gridParametersWidget->updateValue( m_parGrid );
    GSLibParGrid* par21 = m_gpf_sisim->getParameter<GSLibParGrid*>(21 + offset);
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
    GSLibParMultiValuedFixed *par32 = m_gpf_sisim->getParameter<GSLibParMultiValuedFixed*>(32 + offset);
    if( ui->radioFullIK->isChecked() )
        par32->getParameter<GSLibParOption*>(0)->_selected_value = 0;
    else
        par32->getParameter<GSLibParOption*>(0)->_selected_value = 1;

	///////// Adjust offset to account for one less parameter yet for sisim_lm (kriging type)/////////////////////
	if ( sisimProgram == "sisim_lm" )
		offset = -4;
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //Variogram models for each threshold/category of one variogram model if IK is in median mode.
    GSLibParRepeat *par34 = m_gpf_sisim->getParameter<GSLibParRepeat*>(34 + offset);
    if( ui->radioMedianIK->isChecked() ){
//		par34->setCount( 1 );
//		GSLibParVModel *par34_0 = par34->getParameter<GSLibParVModel*>(0, 0);
//		VariogramModelSelector* vms = m_variogramSelectors.at( 0 );
//		VariogramModel *vmodel = vms->getSelectedVModel();
//		par34_0->setFromVariogramModel( vmodel );
		// SISIM should expect only one variogram for mIK... anyway, we repeat the same
		// variogram for all thresholds...
		par34->setCount( nThresholdsOrCategories );
		for( uint i = 0; i < nThresholdsOrCategories; ++i){
			GSLibParVModel *par34_0 = par34->getParameter<GSLibParVModel*>(i, 0);
			VariogramModelSelector* vms = m_variogramSelectors.at( 0 );
			VariogramModel *vmodel = vms->getSelectedVModel();
			par34_0->setFromVariogramModel( vmodel );
		}
	} else {
		par34->setCount( nThresholdsOrCategories );
        for( uint i = 0; i < nThresholdsOrCategories; ++i){
            GSLibParVModel *par34_0 = par34->getParameter<GSLibParVModel*>(i, 0);
            VariogramModelSelector* vms = m_variogramSelectors.at( i );
            VariogramModel *vmodel = vms->getSelectedVModel();
            par34_0->setFromVariogramModel( vmodel );
        }
	}

	//----------------------------prepare and execute the SISIM program (sisim, sisim_gs or sisim_lm)--------------------------------
    //show the sisim parameters
    GSLibParametersDialog gsd( m_gpf_sisim, this );
    int result = gsd.exec();

    //if user didn't cancel the dialog
    if( result == QDialog::Accepted ){
        //Generate the parameter file
        QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "par" );
        m_gpf_sisim->save( par_file_path );

        //to be notified when sisim completes.
        connect( GSLib::instance(), SIGNAL(programFinished()), this, SLOT(onSisimCompletes()) );

        //run sisim program asynchronously
        Application::instance()->logInfo("Starting " + sisimProgram + " program...");
        GSLib::instance()->runProgramAsync( sisimProgram, par_file_path );
    }
}

void SisimDialog::onSisimCompletes()
{
    //frees all signal connections to the GSLib singleton.
    GSLib::instance()->disconnect();

    preview();
}

void SisimDialog::onRealizationHistogram()
{
    //Get the Cartesian grid object.
    CartesianGrid* cg = m_cg_simulation;

    //ask the user for the realization number to select
    bool ok = false;
    uint nreal = cg->getNReal();
    int realNumber = QInputDialog::getInt(this, "Set realization number",
                      "realization number (count=" + QString::number(nreal) + "):", 1, 1, nreal, 1,
                       &ok);
    if(!ok) return;

    //construct a parameter file for the addcoord program
    //We use the addcoord program to extract data from a specific realization
    //for the histogram
    GSLibParameterFile gpfForAddcoord( "addcoord" );

    //input file
    GSLibParFile* par0 = gpfForAddcoord.getParameter<GSLibParFile*>( 0 );
    par0->_path = cg->getPath();

    //output file (temporary)
    GSLibParFile* par1 = gpfForAddcoord.getParameter<GSLibParFile*>( 1 );
    par1->_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "xyz" );

    //realization number
    GSLibParUInt* par2 = gpfForAddcoord.getParameter<GSLibParUInt*>( 2 );
    par2->_value = realNumber;

    //grid parameters
    GSLibParGrid* par3 = gpfForAddcoord.getParameter<GSLibParGrid*>( 3 );
    par3->setFromCG( cg );

    //Generate the parameter file for the addcoord program
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    gpfForAddcoord.save( par_file_path );

    //run addcoord program
    Application::instance()->logInfo("Starting addcoord program...");
    GSLib::instance()->runProgram( "addcoord", par_file_path );
    Application::instance()->logInfo("addcoord completed.");

    //make the point set object from the temporary point set file created by addcoord
    //TODO: memory leak. delete the object pointed by ps somewhere (destructor?)
    PointSet *ps = new PointSet( par1->_path );

    //addcoord always adds the X,Y,Z fields as 1st, 2nd and 3rd variables in the data file
    //the ndv value is the same as the original Cartesian grid.
    ps->setInfo( 1, 2, 3, cg->getNoDataValue() );

    //the simulated values of the target realization is the 4th GEO-EAS column in the point set file
    //generated by addcord
    Attribute *at = ps->getAttributeFromGEOEASIndex( 4 );
    Util::viewHistogram( at, this );
}

void SisimDialog::onEnsembleHistogram()
{
    QString sisimProgram = ui->cmbProgram->currentText();

    ///////// Set an offset to account for differences in parameter count between the different sisim programs/////////////////////
    int offset = 0;
    if( sisimProgram == "sisim" )
        offset = 0;
    else if ( sisimProgram == "sisim_gs" )
        offset = -1;
	else if ( sisimProgram == "sisim_lm" )
		offset = -3;
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //get simulation data
    CartesianGrid* gridRealizations = m_cg_simulation;
    Attribute* realizationsAttribute = m_cg_simulation->getAttributeFromGEOEASIndex(1);

    //load the data in grid
    gridRealizations->loadData();

    //get the maximum and minimun of selected variable, excluding no-data value
    double data_min = gridRealizations->min( realizationsAttribute->getAttributeGEOEASgivenIndex()-1 );
    double data_max = gridRealizations->max( realizationsAttribute->getAttributeGEOEASgivenIndex()-1 );
    data_min -= std::fabs( data_min / 100.0 );
    data_max += std::fabs( data_max / 100.0 );

    //------------------------------parameters setting--------------------------------------

    GSLibParameterFile gpf( "histpltsim" );

    //Set default values so we need to change less parameters and let
    //the user change the others as one may see fit.
    gpf.setDefaultValues();

    //Use the input data to provide a reference distribution
    GSLibParMultiValuedFixed* par3 = gpf.getParameter<GSLibParMultiValuedFixed*>(3);
    GSLibParMultiValuedFixed* sisim_par5 = m_gpf_sisim->getParameter<GSLibParMultiValuedFixed*>(5);
    gpf.getParameter<GSLibParFile*>(2)->_path = m_gpf_sisim->getParameter<GSLibParFile*>(4)->_path;
    //   columns for reference variable and weight
    par3->getParameter<GSLibParUInt*>(0)->_value = sisim_par5->getParameter<GSLibParUInt*>(3)->_value;
    par3->getParameter<GSLibParUInt*>(1)->_value = 0;

    //file with distributions to check (file with the realizations)
    gpf.getParameter<GSLibParFile*>(4)->_path = m_gpf_sisim->getParameter<GSLibParFile*>(19 + offset)->_path;

    //   columns for variable and weight
    GSLibParMultiValuedFixed* par5 = gpf.getParameter<GSLibParMultiValuedFixed*>(5);
    par5->getParameter<GSLibParUInt*>(0)->_value = 1;
    par5->getParameter<GSLibParUInt*>(1)->_value = 0;

    //   start and finish histograms (usually 1 and nreal)
    GSLibParMultiValuedFixed* par7 = gpf.getParameter<GSLibParMultiValuedFixed*>(7);
    par7->getParameter<GSLibParUInt*>(0)->_value = 1;
    par7->getParameter<GSLibParUInt*>(1)->_value = m_gpf_sisim->getParameter<GSLibParUInt*>(20 + offset)->_value ;

    //   nx, ny, nz
    GSLibParMultiValuedFixed* par8 = gpf.getParameter<GSLibParMultiValuedFixed*>(8);
    par8->getParameter<GSLibParUInt*>(0)->_value = gridRealizations->getNX();
    par8->getParameter<GSLibParUInt*>(1)->_value = gridRealizations->getNY();
    par8->getParameter<GSLibParUInt*>(2)->_value = gridRealizations->getNZ();

    //   trimming limits
    GSLibParMultiValuedFixed* par9 = gpf.getParameter<GSLibParMultiValuedFixed*>(9);
    par9->getParameter<GSLibParDouble*>(0)->_value = data_min;
    par9->getParameter<GSLibParDouble*>(1)->_value = data_max;

    //file for PostScript output
    gpf.getParameter<GSLibParFile*>(10)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("ps");

    //file for summary output (always used)
    gpf.getParameter<GSLibParFile*>(11)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("txt");

    //file for numeric output (used if flag set above)
    gpf.getParameter<GSLibParFile*>(12)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");;

    //attribute minimum and maximum
    GSLibParMultiValuedFixed* par13 = gpf.getParameter<GSLibParMultiValuedFixed*>(13);
    par13->getParameter<GSLibParDouble*>(0)->_value = data_min;
    par13->getParameter<GSLibParDouble*>(1)->_value = data_max;

    //title
    QString title = m_InputPointSetFileSelector->getSelectedDataFile()->getName() + "/" +
            m_InputVariableSelector->getSelectedVariableName() + ": SISIM";
    gpf.getParameter<GSLibParString*>(18)->_value = title;

    //reference value for box plot
    gpf.getParameter<GSLibParDouble*>(20)->_value = gridRealizations->mean( realizationsAttribute->getAttributeGEOEASgivenIndex()-1 );

    //----------------------------------------------------------------------------------

    //Generate the parameter file
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    gpf.save( par_file_path );

    //run histpltsim program
    Application::instance()->logInfo("Starting histpltsim program...");
    GSLib::instance()->runProgram( "histpltsim", par_file_path );

    //display the plot output
    DisplayPlotDialog *dpd = new DisplayPlotDialog(gpf.getParameter<GSLibParFile*>(10)->_path, title, gpf, this);
    dpd->show(); //show() makes dialog modalless

}

void SisimDialog::onEnsembleVariogram()
{
    //get the input variable's variogram model (it is not one of the indicator variograms)
    QString inputVariableName = m_InputVariableSelector->getSelectedVariableName();
    VariogramInputDialog vid( this );
    vid.setCaption( "Enter variogram of " + inputVariableName + ". It is not one of the indicator variograms.");
    int resultVid = vid.exec();
    VariogramModel* variogramModelInputVariable = nullptr;
    if( resultVid == QDialog::Accepted ){
        variogramModelInputVariable = vid.getSelectedVariogramModel();
        if( ! variogramModelInputVariable ){
            Application::instance()->logError("SisimDialog::onEnsembleVariogram(): null variogram model.  Aborted.");
            return;
        }
    } else {
        return; //Abort
    }

    //get the number of realizations
    uint nReals = m_cg_simulation->getNReal();

    //-------------------------------------------------------------------------------------------
    //-----------1) Run gam on each realization--------------------------------------------------
    //-------------------------------------------------------------------------------------------

    //if the parameter file object was not constructed
    if( ! m_gpf_gam ){

        //Construct an object composition based on the parameter file template for the gam program.
        m_gpf_gam = new GSLibParameterFile( "gam" );

        //get input data file
        //the parent component of an attribute is a file
        CartesianGrid* input_data_file = m_cg_simulation;

        //loads data from file.
        input_data_file->loadData();

        //get the variable index in parent data file (always 1 in the case of simulations)
        uint var_index = 1;

        //Set default values so we need to change less parameters and let
        //the user change the others as one may see fit.
        m_gpf_gam->setDefaultValues();

        //get the max and min of the selected variable(s)
        double data_min = input_data_file->min( var_index-1 );
        double data_max = input_data_file->max( var_index-1 );

        //----------------set the minimum required gam paramaters-----------------------
        //input file
        m_gpf_gam->getParameter<GSLibParFile*>(0)->_path = input_data_file->getPath();

        //variable to compute variogram for
        GSLibParMultiValuedFixed *par1 = m_gpf_gam->getParameter<GSLibParMultiValuedFixed*>(1);
        par1->getParameter<GSLibParUInt*>(0)->_value = 1; //number of variables
        GSLibParMultiValuedVariable *par1_1 = par1->getParameter<GSLibParMultiValuedVariable*>(1);
        par1_1->getParameter<GSLibParUInt*>(0)->_value = var_index;

        //trimming limits
        GSLibParMultiValuedFixed *par2 = m_gpf_gam->getParameter<GSLibParMultiValuedFixed*>(2);
        par2->getParameter<GSLibParDouble*>(0)->_value = data_min;
        par2->getParameter<GSLibParDouble*>(1)->_value = data_max;

        //output file with experimental variogram values
        m_gpf_gam->getParameter<GSLibParFile*>(3)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("out");

        //grid definition parameters
        GSLibParGrid* par5 = m_gpf_gam->getParameter<GSLibParGrid*>(5);
        par5->_specs_x->getParameter<GSLibParUInt*>(0)->_value = input_data_file->getNX(); //nx
        par5->_specs_x->getParameter<GSLibParDouble*>(1)->_value = input_data_file->getX0(); //min x
        par5->_specs_x->getParameter<GSLibParDouble*>(2)->_value = input_data_file->getDX(); //cell size x
        par5->_specs_y->getParameter<GSLibParUInt*>(0)->_value = input_data_file->getNY(); //ny
        par5->_specs_y->getParameter<GSLibParDouble*>(1)->_value = input_data_file->getY0(); //min y
        par5->_specs_y->getParameter<GSLibParDouble*>(2)->_value = input_data_file->getDY(); //cell size y
        par5->_specs_z->getParameter<GSLibParUInt*>(0)->_value = input_data_file->getNZ(); //nz
        par5->_specs_z->getParameter<GSLibParDouble*>(1)->_value = input_data_file->getZ0(); //min z
        par5->_specs_z->getParameter<GSLibParDouble*>(2)->_value = input_data_file->getDZ(); //cell size z

    }
    //--------------------------------------------------------------------------------

    //show the parameter dialog so the user can adjust other settings before running gam
    GSLibParametersDialog gslibpardiag( m_gpf_gam );
    int result = gslibpardiag.exec();
    std::vector<QString> expVarFilePaths;
    if( result == QDialog::Accepted ){
        //save the realization number setting for the variogram modeling workflow
        int oldNReal = m_gpf_gam->getParameter<GSLibParUInt*>(4)->_value;
        //for each realization number...
        for( uint iRealNum = 0 ; iRealNum < nReals; ++iRealNum ){
            //...change the realization number parameter for gam
            m_gpf_gam->getParameter<GSLibParUInt*>(4)->_value = iRealNum + 1;
            //...set an output file with experimental variogram values
            m_gpf_gam->getParameter<GSLibParFile*>(3)->_path =
                    Application::instance()->getProject()->generateUniqueTmpFilePath("out");
            expVarFilePaths.push_back( m_gpf_gam->getParameter<GSLibParFile*>(3)->_path );
            //...Generate the parameter file
            QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
            m_gpf_gam->save( par_file_path );
            //...run gam program
            Application::instance()->logInfo("Starting gam program for realization " +
                                             QString::number(iRealNum + 1) + "...");
            GSLib::instance()->runProgram( "gam", par_file_path );
        }
        //restore the realization number setting for the variogram modeling workflow
        m_gpf_gam->getParameter<GSLibParUInt*>(4)->_value = oldNReal;
    }

    //---------------------------------------------------------------------------------------------------------------
    //-------------------------- 2) Run vmodel to generate the variogram model (reference)---------------------------
    //---------------------------------------------------------------------------------------------------------------

    //these are useful to compute lag, azimuth and dip from gam regular grid parameters
    double xsize = 1.0;
    double ysize = 1.0;
    double zsize = 1.0;
    if( m_gpf_gam )
    {
        GSLibParGrid* par5 = m_gpf_gam->getParameter<GSLibParGrid*>(5);
        xsize = par5->_specs_x->getParameter<GSLibParDouble*>(2)->_value; //cell size x
        ysize = par5->_specs_y->getParameter<GSLibParDouble*>(2)->_value; //cell size y
        zsize = par5->_specs_z->getParameter<GSLibParDouble*>(2)->_value; //cell size z
    }

    //Construct an object composition based on the parameter file template for the vmodel program.
    GSLibParameterFile gpf_vmodel = GSLibParameterFile( "vmodel" );
    //Set default values so we need to change less parameters and let
    //the user change the others as one may see fit.
    gpf_vmodel.setDefaultValues();

    //fills the variogram model paramaters.
    gpf_vmodel.setValuesFromParFile( variogramModelInputVariable->getPath() );

    //output variography data for vargplt
    gpf_vmodel.getParameter<GSLibParFile*>(0)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("var");

    //match the number of lags and azimuths with that set for the gam on the realizations
    GSLibParMultiValuedFixed *gam_par6 = m_gpf_gam->getParameter<GSLibParMultiValuedFixed*>(6);
    GSLibParMultiValuedFixed *par1 = gpf_vmodel.getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = gam_par6->getParameter<GSLibParUInt*>(0)->_value; //ndir
    par1->getParameter<GSLibParUInt*>(1)->_value = gam_par6->getParameter<GSLibParUInt*>(1)->_value;

    //get the number of directions
    uint ndir = gam_par6->getParameter<GSLibParUInt*>(0)->_value;

    //compute azimuth and dip from the grid cell dimensions and steps set in gam
    GSLibParRepeat *gam_par7 = m_gpf_gam->getParameter<GSLibParRepeat*>(7); //repeat ndir-times
    QList<double> azimuths;
    QList<double> dips;
    QList<double> lags;
    for(uint i = 0; i < ndir; ++i)
    {
        GSLibParMultiValuedFixed *par7_0 = gam_par7->getParameter<GSLibParMultiValuedFixed*>(i, 0);
        int xstep = par7_0->getParameter<GSLibParInt*>(0)->_value;
        int ystep = par7_0->getParameter<GSLibParInt*>(1)->_value;
        int zstep = par7_0->getParameter<GSLibParInt*>(2)->_value;
        azimuths.append( Util::getAzimuth( xsize, ysize, xstep, ystep ) );
        dips.append( Util::getDip(xsize, ysize, zsize, xstep, ystep, zstep) );
        double xlag = xstep * xsize;
        double ylag = ystep * ysize;
        double zlag = zstep * zsize;
        double lag = std::sqrt( xlag*xlag + ylag*ylag + zlag*zlag );
        lags.append( lag );
    }

    //match the azimuths, dips and lags with those of the gam
    GSLibParRepeat *par2 = gpf_vmodel.getParameter<GSLibParRepeat*>(2); //repeat ndir-times
    par2->setCount( ndir );
    for( uint i = 0; i < ndir; ++i)
    {
        GSLibParMultiValuedFixed *par2_0 = par2->getParameter<GSLibParMultiValuedFixed*>(i, 0);
        par2_0->getParameter<GSLibParDouble*>(0)->_value = azimuths[i];
        par2_0->getParameter<GSLibParDouble*>(1)->_value = dips[i];
        par2_0->getParameter<GSLibParDouble*>(2)->_value = lags[i];
    }

    //Generate the vmodel parameter file
    QString vmodel_par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    gpf_vmodel.save( vmodel_par_file_path );
    //run vmodel program )
    Application::instance()->logInfo("Starting vmodel program...");
    GSLib::instance()->runProgram( "vmodel", vmodel_par_file_path );

    //-------------------------------------------------------------------------------------------
    //-------------------------- 3) Run vargplt to show the variograms---------------------------
    //-------------------------------------------------------------------------------------------

    //make a GLSib parameter object if it wasn't done yet.
    GSLibParameterFile gpf = GSLibParameterFile( "vargplt" );

    //Set default values so we need to change less parameters and let
    //the user change the others as one may see fit.
    gpf.setDefaultValues();

    //make plot/window title
    QString title = m_InputPointSetFileSelector->getSelectedDataFile()->getName() + "/" +
            m_InputVariableSelector->getSelectedVariableName() + ": SISIM";

    //--------------------set some parameter values-----------------------

    //postscript file
    gpf.getParameter<GSLibParFile*>(0)->_path =
            Application::instance()->getProject()->generateUniqueTmpFilePath("ps");

    //number of curves
    gpf.getParameter<GSLibParUInt*>(1)->_value = nReals + 1; // nvarios (realizations + variogram model)

    //plot title
    gpf.getParameter<GSLibParString*>(5)->_value = title;

    //suggest display settings for each variogram curve
    GSLibParRepeat *par6 = gpf.getParameter<GSLibParRepeat*>(6); //repeat nvarios-times
    par6->setCount( nReals + 1 ); //the realizations plus the variogram model (reference)
    //the realization curves
    for(uint iReal = 0; iReal < nReals; ++iReal){
        par6->getParameter<GSLibParFile*>(iReal, 0)->_path = expVarFilePaths[iReal];
        GSLibParMultiValuedFixed *par6_0_1 = par6->getParameter<GSLibParMultiValuedFixed*>(iReal, 1);
        par6_0_1->getParameter<GSLibParUInt*>(0)->_value = 1;
        par6_0_1->getParameter<GSLibParUInt*>(1)->_value = 0;
        par6_0_1->getParameter<GSLibParOption*>(2)->_selected_value = 0;
        par6_0_1->getParameter<GSLibParOption*>(3)->_selected_value = 1;
        par6_0_1->getParameter<GSLibParColor*>(4)->_color_code = 1;
    }
    par6->getParameter<GSLibParFile*>(nReals, 0)->_path = gpf_vmodel.getParameter<GSLibParFile*>(0)->_path;
    GSLibParMultiValuedFixed *par6_0_1 = par6->getParameter<GSLibParMultiValuedFixed*>(nReals, 1);
    par6_0_1->getParameter<GSLibParUInt*>(0)->_value = 1;
    par6_0_1->getParameter<GSLibParUInt*>(1)->_value = 0;
    par6_0_1->getParameter<GSLibParOption*>(2)->_selected_value = 1;
    par6_0_1->getParameter<GSLibParOption*>(3)->_selected_value = 0;
    par6_0_1->getParameter<GSLibParColor*>(4)->_color_code = 10;
    //the variogram model curve


    //----------------------display plot------------------------------------------------------------

    //Generate the parameter file
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    gpf.save( par_file_path );

    //run vargplt program
    Application::instance()->logInfo("Starting vargplt program...");
    GSLib::instance()->runProgram( "vargplt", par_file_path );

    //display the plot output
    DisplayPlotDialog *dpd = new DisplayPlotDialog(gpf.getParameter<GSLibParFile*>(0)->_path,
                                                   gpf.getParameter<GSLibParString*>(5)->_value,
                                                   gpf,
                                                   this);
    dpd->show();


}

void SisimDialog::onSaveEnsemble()
{
    bool ok;
    //propose a name based on the point set name.
    QString proposed_name( m_InputVariableSelector->getSelectedVariableName() + "_SISIM" );
    proposed_name.append( ".grid" );
    QString new_cg_name = QInputDialog::getText(this, "Name the new grid file",
                                             "New grid file name:", QLineEdit::Normal,
                                             proposed_name, &ok);

    if (ok && !new_cg_name.isEmpty()){

        if( m_varType == IKVariableType::CATEGORICAL ){
            //get the selected p.d.f. file
            CategoryPDF *pdf = (CategoryPDF *)m_DensityFunctionSelector->getSelectedFile();
            //get the category definition
            CategoryDefinition* cd = pdf->getCategoryDefinition();
            //Get the simulated values
            std::vector< double > values = m_cg_simulation->getDataColumn(0);
            //Duplicate it as a new column with the categorical definition
            m_cg_simulation->addNewDataColumn( m_InputVariableSelector->getSelectedVariableName(),
                                               values,
                                               cd);
            //write to file
            m_cg_simulation->saveData();
            //remove the first column not linked to a categorical definition
            m_cg_simulation->deleteVariable(0);
        }

        //import the newly created grid file as a project item
        Application::instance()->getProject()->importCartesianGrid( m_cg_simulation, new_cg_name );
	}
}

void SisimDialog::onPostsim()
{
	//load the data in grid
	m_cg_simulation->loadData();

	//get the maximum and minimun of simulated values
	double data_min = m_cg_simulation->min( 0 );
	double data_max = m_cg_simulation->max( 0 );
	data_min -= std::fabs( data_min / 100.0 );
	data_max += std::fabs( data_max / 100.0 );

	//create the parameters object if not created
	if( ! m_gpf_postsim ){
		m_gpf_postsim = new GSLibParameterFile("postsim");
		//file for output array(s)
		m_gpf_postsim->getParameter<GSLibParFile*>(4)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");
	}

	//file with simulated realizations
	m_gpf_postsim->getParameter<GSLibParFile*>(0)->_path = m_cg_simulation->getPath();
	//   number of realizations
	m_gpf_postsim->getParameter<GSLibParUInt*>(1)->_value = m_cg_simulation->getNReal();
	//   trimming limits
	GSLibParMultiValuedFixed* par2 = m_gpf_postsim->getParameter<GSLibParMultiValuedFixed*>(2);
	par2->getParameter<GSLibParDouble*>(0)->_value = data_min;
	par2->getParameter<GSLibParDouble*>(1)->_value = data_max;
	//nx, ny, nz
	GSLibParMultiValuedFixed* par3 = m_gpf_postsim->getParameter<GSLibParMultiValuedFixed*>(3);
	par3->getParameter<GSLibParUInt*>(0)->_value = m_cg_simulation->getNX();
	par3->getParameter<GSLibParUInt*>(1)->_value = m_cg_simulation->getNY();
	par3->getParameter<GSLibParUInt*>(2)->_value = m_cg_simulation->getNZ();

	//show the postsim parameters
	GSLibParametersDialog gsd( m_gpf_postsim, this );
	int result = gsd.exec();

	//if user didn't cancel the dialog
	if( result == QDialog::Accepted ){
		//Generate the parameter file
		QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "par" );
		m_gpf_postsim->save( par_file_path );

		//run postsim program asynchronously
		Application::instance()->logInfo("Starting postsim program...");
		GSLib::instance()->runProgram( "postsim", par_file_path );

		previewPostsim();
	}
}

void SisimDialog::onSavePostsim()
{
	bool ok;

	//part of the suggested name for the grid depends on the post-processed product type
	GSLibParMultiValuedFixed* postsim_par5 = m_gpf_postsim->getParameter<GSLibParMultiValuedFixed*>(5);
	QString postsimType;
	switch( postsim_par5->getParameter<GSLibParOption*>(0)->_selected_value ){
	case 1: postsimType = "EType"; break;
	case 2: postsimType = "ProbMeanAb" + QString::number(postsim_par5->getParameter<GSLibParDouble*>(1)->_value); break;
	case 3: postsimType = "P" + QString::number(postsim_par5->getParameter<GSLibParDouble*>(1)->_value * 100); break;
	case 4: postsimType = "SymmProbIntervFor" + QString::number(postsim_par5->getParameter<GSLibParDouble*>(1)->_value);
	}

	//propose a name based the postsim product selected by the user.
	QString proposed_name( m_InputVariableSelector->getSelectedVariableName() + "_SISIM_" + postsimType );
	proposed_name.append( ".grid" );
	QString new_cg_name = QInputDialog::getText(this, "Name the new grid file",
											 "New grid file name:", QLineEdit::Normal,
											 proposed_name, &ok);

	if (ok && !new_cg_name.isEmpty()){
		//import the newly created grid file as a project item
		Application::instance()->getProject()->importCartesianGrid( m_cg_postsim, new_cg_name );
    }
}

void SisimDialog::onSisimProgramChanged(QString)
{
    configureSoftDataUI();
    //destroy the current object pointed by m_gpf_sisim and reset the pointer
    //this will force onConfigureAndRun() to build a new one with a different set of parameters.
    if( m_gpf_sisim )
        delete m_gpf_sisim;
    m_gpf_sisim = nullptr;

}
