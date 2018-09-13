#include "factorialkrigingdialog.h"
#include "ui_factorialkrigingdialog.h"
#include "domain/application.h"
#include "domain/pointset.h"
#include "domain/project.h"
#include "domain/cartesiangrid.h"
#include "domain/variogrammodel.h"
#include "domain/attribute.h"
#include "widgets/variogrammodelselector.h"
#include "widgets/cartesiangridselector.h"
#include "widgets/pointsetselector.h"
#include "widgets/variableselector.h"
#include "widgets/fileselectorwidget.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslibparametersdialog.h"
#include "gslib/gslib.h"
#include "util.h"
#include "geostats/fkestimation.h"
#include "geostats/searchstrategy.h"
#include "geostats/searchellipsoid.h"

#include <QInputDialog>
#include <QMessageBox>
#include <cmath>

FactorialKrigingDialog::FactorialKrigingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FactorialKrigingDialog),
	m_cg_estimation( nullptr ),
	m_cg_preview( nullptr ),
	m_cg_nSamples( nullptr ),
	m_gpfFK( nullptr )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    //The list with existing variogram models in the project.
    m_vModelSelector = new VariogramModelSelector();
    ui->frmVariogram->layout()->addWidget( m_vModelSelector );
    connect( m_vModelSelector, SIGNAL(variogramSelected()),
             this, SLOT(onVariogramChanged()) );

    //The list with existing cartesian grids in the project for the estimation.
	m_cgSelector = new CartesianGridSelector();
    ui->frmGrid->layout()->addWidget( m_cgSelector );

	//The list with existing data sets in the project.
	m_dataSetSelector = new FileSelectorWidget( FileSelectorType::DataFiles );
	ui->frmData->layout()->addWidget( m_dataSetSelector );
	connect( m_dataSetSelector, SIGNAL(dataFileSelected(DataFile*)),
			 this, SLOT(onDataSetSelected(DataFile*)) );

    //The list with the Point Set variables to set the primary variable
	m_DataSetVariableSelector = new VariableSelector();
	ui->frmData->layout()->addWidget( m_DataSetVariableSelector );
	connect( m_dataSetSelector, SIGNAL(dataFileSelected(DataFile*)),
			 m_DataSetVariableSelector, SLOT(onListVariables(DataFile*)) );

    //calling this slot causes the variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
	m_dataSetSelector->onSelection( 0 );

	if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI ){
        ui->btnParameters->setIcon( QIcon(":icons32/setting32") );
        ui->btnSave->setIcon( QIcon(":icons32/save32") );
    }

    adjustSize();
}

FactorialKrigingDialog::~FactorialKrigingDialog()
{
	if( m_gpfFK )
		delete m_gpfFK;
	if( m_cg_preview )
		delete m_cg_preview;
    delete ui;
    Application::instance()->logInfo("FactorialKrigingDialog destroyed.");
}

void FactorialKrigingDialog::onParameters()
{

    //-------------------------gather estimation info-----------------------------

    //get the selected input file
	DataFile* input_data_file = static_cast<DataFile*>( m_dataSetSelector->getSelectedFile() );
    if( ! input_data_file ){
		QMessageBox::critical( this, "Error", "Please, select a data set data file.");
        return;
    }
    input_data_file->loadData();

    //get the selected estimation grid
    CartesianGrid* estimation_grid = (CartesianGrid*)m_cgSelector->getSelectedDataFile();
    if( ! estimation_grid ){
        QMessageBox::critical( this, "Error", "Please, select an estimation grid.");
        return;
    }

    //get min and max of variable
	double data_min = input_data_file->min( m_DataSetVariableSelector->getSelectedVariableGEOEASIndex()-1 );
	double data_max = input_data_file->max( m_DataSetVariableSelector->getSelectedVariableGEOEASIndex()-1 );
    data_min -= fabs( data_min/100.0 );
    data_max += fabs( data_max/100.0 );

    //get the selected variogram model
    VariogramModel* variogram = m_vModelSelector->getSelectedVModel();
    if( ! variogram ){
        QMessageBox::critical( this, "Error", "Please, select a variogram model.");
        return;
    }

	if( ! m_gpfFK ){
		m_gpfFK = new GSLibParameterFile();
		m_gpfFK->makeParamatersForFactorialKriging();
	}

	//update the available factor options.
    GSLibParOption* factor_par = m_gpfFK->getParameter<GSLibParOption*>( 7 ); // See parameter indexes and types in GSLibParameterFile::makeParamatersForFactorialKriging()
	factor_par->_options.clear();
	factor_par->addOption( -1, "Mean (Factor 1)" );
	factor_par->addOption( 0, "Nugget effect (Factor 2)" );
	for( uint ist = 0;  ist < variogram->getNst(); ++ist){
		factor_par->addOption( ist+1, variogram->getStructureDescription( ist ) +
							   "(Factor " + QString::number(3+ist) + ")" );
	}

	GSLibParametersDialog gpd( m_gpfFK );
	int response = gpd.exec();

	//if user didn't cancel the dialog
	if( response == QDialog::Accepted ){
		//run factorial kriging.
		doFK();
	}

}

void FactorialKrigingDialog::onSave()
{
    //TODO: this onSave() method (or the functions it calls) crashed once.
	if( ! m_cg_estimation ){
        QMessageBox::critical( this, "Error", "Please, run the estimation at least once.");
        return;
    }

	//user enters the name for the new variable with the desired factor.
	QString new_variable_name = QInputDialog::getText(this, "Name the new variable",
											 "Name for the variable with estimates:", QLineEdit::Normal,
											 m_varName );

	//if the user canceled the input box
	if ( new_variable_name.isEmpty() ){
		//abort
		return;
	}

	//add the new data column to the estimation grid selected by the user.
	m_cg_estimation->addNewDataColumn( new_variable_name, m_results );
}

void FactorialKrigingDialog::onSaveEstimates()
{
	this->onSave( );
}

void FactorialKrigingDialog::onVariogramChanged()
{
}

void FactorialKrigingDialog::onDataSetSelected(DataFile * dataFile)
{
//	if( dataFile ){
//		if( dataFile->getFileType() == "CARTESIANGRID" )
//			ui->frmGrid->hide();
//		else
//			ui->frmGrid->show();
//	} else
//		ui->frmGrid->hide();
}

void FactorialKrigingDialog::onMapNSamples()
{
	if( m_vNSamplesAsDoubles.empty() ){
		Application::instance()->logError("FactorialKrigingDialog::onMapNSamples(): no results.  Execute FK at least once.");
		return;
	}

	if( m_cg_nSamples )
		delete m_cg_nSamples;

	//get the tmp file path for the estimates
	QString grid_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");

	//create a new grid object corresponding to the file to be saved
	m_cg_nSamples = new CartesianGrid( grid_file_path );

	//set the grid geometry info.
	m_cg_nSamples->setInfoFromOtherCG( m_cg_estimation );

	//create the physical GEO-EAS grid file with one column.
	Util::createGEOEAScheckerboardGrid( m_cg_nSamples, grid_file_path );

	//calling this again to update the variable collection, now that we have a physical file
	m_cg_nSamples->setInfoFromOtherCG( m_cg_estimation );

	//append a column with the numbers of samples
	m_cg_nSamples->addNewDataColumn( "Number of samples", m_vNSamplesAsDoubles );

	//get the variable with the number of samples (the second column)
	Attribute* nSamples_var = (Attribute*)m_cg_nSamples->getChildByIndex( 1 );

	//open the plot dialog
	Util::viewGrid( nSamples_var, this );
}

void FactorialKrigingDialog::onHistoNSamples()
{
	if( m_vNSamplesAsDoubles.empty() ){
		Application::instance()->logError("FactorialKrigingDialog::onHistoNSamples(): no results.  Execute FK at least once.");
		return;
	}

	if( m_cg_nSamples )
		delete m_cg_nSamples;

	//get the tmp file path for the estimates
	QString grid_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");

	//create a new grid object corresponding to the file to be saved
	m_cg_nSamples = new CartesianGrid( grid_file_path );

	//set the grid geometry info.
	m_cg_nSamples->setInfoFromOtherCG( m_cg_estimation );

	//create the physical GEO-EAS grid file with one column.
	Util::createGEOEAScheckerboardGrid( m_cg_nSamples, grid_file_path );

	//calling this again to update the variable collection, now that we have a physical file
	m_cg_nSamples->setInfoFromOtherCG( m_cg_estimation );

	//append a column with the numbers of samples
	m_cg_nSamples->addNewDataColumn( "Number of samples", m_vNSamplesAsDoubles );

	//get the variable with the number of samples (the second column)
	Attribute* nSamples_var = (Attribute*)m_cg_nSamples->getChildByIndex( 1 );

	//show the histogram of the number of samples
	Util::viewHistogram( nSamples_var, this );
}

void FactorialKrigingDialog::preview()
{
}

void FactorialKrigingDialog::doFK()
{
    //Get the factor number (-1 = mean, 0 = nugget, 1 and onwards = the variogram structures).
    int factor_number =  (m_gpfFK->getParameter<GSLibParOption*>( 7 ))->_selected_value;

    //propose a name for the new variable to contain the choosen factor.
	QString factorName;
	VariogramModel* vModel = m_vModelSelector->getSelectedVModel();
	switch( factor_number ){
	case -1: factorName = "mean"; break;
	case 0: factorName = "nugget"; break;
	default: factorName = vModel->getStructureDescription( factor_number - 1 );
	}
	QString kTypeName;
	switch ( static_cast<KrigingType>(m_gpfFK->getParameter<GSLibParOption*>( 0 )->_selected_value) ) {
	case KrigingType::OK: kTypeName = "OFK"; break;
	case KrigingType::SK: kTypeName = "SFK"; break;
	default: kTypeName = "FK";
	}
	QString tmp_name = m_DataSetVariableSelector->getSelectedVariableName() + "_" + kTypeName + "_" + factorName;
	tmp_name = tmp_name.replace('(', ' ');
	tmp_name = tmp_name.replace(')', ' ');
	m_varName = tmp_name;

	// Get the estimation grid.
	m_cg_estimation = static_cast<CartesianGrid*>( m_cgSelector->getSelectedDataFile() );

    //Build the search strategy and search neighborhood objects from the user-input values.
	// See parameter indexes and types in GSLibParameterFile::makeParamatersForFactorialKriging()
    GSLibParMultiValuedFixed* search_ellip_radii_par = m_gpfFK->getParameter<GSLibParMultiValuedFixed*>( 4 );
	double hMax    =  search_ellip_radii_par->getParameter<GSLibParDouble*>(0)->_value;
	double hMin    =  search_ellip_radii_par->getParameter<GSLibParDouble*>(1)->_value;
	double hVert   =  search_ellip_radii_par->getParameter<GSLibParDouble*>(2)->_value;
    GSLibParMultiValuedFixed* search_ellip_angles_par = m_gpfFK->getParameter<GSLibParMultiValuedFixed*>( 5 );
	double azimuth = search_ellip_angles_par->getParameter<GSLibParDouble*>(0)->_value;
    double dip     = search_ellip_angles_par->getParameter<GSLibParDouble*>(1)->_value;
	double roll    = search_ellip_angles_par->getParameter<GSLibParDouble*>(2)->_value;
	uint nb_samples = m_gpfFK->getParameter<GSLibParUInt*>( 2 )->_value;
    GSLibParMultiValuedFixed *par_search_ellip_sectors = m_gpfFK->getParameter<GSLibParMultiValuedFixed*>( 6 );
    uint numberOfSectors = par_search_ellip_sectors->getParameter<GSLibParUInt*>( 0 )->_value;
    uint minSamplesPerSector = par_search_ellip_sectors->getParameter<GSLibParUInt*>( 1 )->_value;
    uint maxSamplesPerSector = par_search_ellip_sectors->getParameter<GSLibParUInt*>( 2 )->_value;
    SearchNeighborhoodPtr searchNeighborhood(
                new SearchEllipsoid(hMax, hMin, hVert,
                                    azimuth, dip, roll,
                                    numberOfSectors, minSamplesPerSector, maxSamplesPerSector
                                    )
                );
    SearchStrategyPtr searchStrategy( new SearchStrategy( searchNeighborhood,
														  nb_samples,
                                                          m_gpfFK->getParameter<GSLibParDouble*>( 8 )->_value, // See parameter indexes and types in GSLibParameterFile::makeParamatersForFactorialKriging()
                                                          m_gpfFK->getParameter<GSLibParUInt*>( 3 )->_value) ); // See parameter indexes and types in GSLibParameterFile::makeParamatersForFactorialKriging()

    //run the estimation
	m_results.clear();
	m_vNSamplesAsDoubles.clear();
    {
        FKEstimation estimation;
        estimation.setSearchStrategy( searchStrategy );
        estimation.setVariogramModel( m_vModelSelector->getSelectedVModel() );
		GSLibParOption* ktype_par = m_gpfFK->getParameter<GSLibParOption*>( 0 ); // See parameter indexes and types in GSLibParameterFile::makeParamatersForFactorialKriging()
        estimation.setKrigingType( static_cast<KrigingType>( ktype_par->_selected_value ) );
		GSLibParDouble* skmean_par = m_gpfFK->getParameter<GSLibParDouble*>( 1 ); // See parameter indexes and types in GSLibParameterFile::makeParamatersForFactorialKriging()
        estimation.setMeanForSimpleKriging( skmean_par->_value );
        estimation.setInputVariable( m_DataSetVariableSelector->getSelectedVariable() );
        estimation.setEstimationGrid( m_cg_estimation );
        estimation.setFactorNumber( factor_number );
		m_results = estimation.run( );
		//get the numbers of sample used in the estimations
		std::vector< uint > vNSamplesAsUints = estimation.getNumberOfSamples();
		std::copy( vNSamplesAsUints.begin(), vNSamplesAsUints.end(), std::back_inserter( m_vNSamplesAsDoubles ) );
    }

	//preview the results
	{
		if( m_cg_preview )
			delete m_cg_preview;

		//get the tmp file path for the estimates
		QString grid_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");

		//create a new grid object corresponding to the file to be saved
		m_cg_preview = new CartesianGrid( grid_file_path );

		//set the grid geometry info.
		m_cg_preview->setInfoFromOtherCG( m_cg_estimation );

		//create the physical GEO-EAS grid file with one column.
		Util::createGEOEAScheckerboardGrid( m_cg_preview, grid_file_path );

		//calling this again to update the variable collection, now that we have a physical file
		m_cg_preview->setInfoFromOtherCG( m_cg_estimation );

		//append a column with the results
		m_cg_preview->addNewDataColumn( m_varName, m_results );

		//get the variable with the estimation values (the second column)
		Attribute* est_var = (Attribute*)m_cg_preview->getChildByIndex( 1 );

		//open the plot dialog
		Util::viewGrid( est_var, this );
	}

}
