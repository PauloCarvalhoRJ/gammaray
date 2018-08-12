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

#include <QInputDialog>
#include <QMessageBox>
#include <cmath>

FactorialKrigingDialog::FactorialKrigingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FactorialKrigingDialog),
	m_cg_estimation( nullptr ),
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
	m_dataSetSelector = new FileSelectorWidget( FileSelectorType::DataFiles, true );
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
        ui->btnSaveOrUpdateVModel->setIcon( QIcon(":icons32/save32") );
    }

    adjustSize();
}

FactorialKrigingDialog::~FactorialKrigingDialog()
{
	if( m_gpfFK )
		delete m_gpfFK;
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

    //get the selected grid with secondary data (if any)
    CartesianGrid* sec_data_grid = (CartesianGrid*)m_cgSelectorSecondary->getSelectedDataFile();

	if( ! m_gpfFK ){
		GSLibParameterFile* m_gpfFK = new GSLibParameterFile();
		m_gpfFK->makeParamatersForFactorialKriging();

		GSLibParametersDialog gpd( m_gpfFK );
		int response = gpd.exec();

		//if user didn't cancel the dialog
		if( response == QDialog::Accepted ){
            //run factorial kriging.
            doFK();
		}
	}

}

void FactorialKrigingDialog::onSave(bool estimates)
{
    //TODO: this onSave() method (or the functions it calls) crashed once.
	if( ! m_cg_estimation ){
        QMessageBox::critical( this, "Error", "Please, run the estimation at least once.");
        return;
    }

    //get the selected estimation grid
    CartesianGrid* estimation_grid = (CartesianGrid*)m_cgSelector->getSelectedDataFile();
    if( ! estimation_grid ){
        QMessageBox::critical( this, "Error", "Please, select an estimation grid.");
        return;
    }

    //suggest a name to the user
	QString proposed_name( m_DataSetVariableSelector->getSelectedVariableName() );
    proposed_name = proposed_name.append( ( estimates ? "_ESTIMATES" : "_KVARIANCES" ) );

    //presents a dialog so the user can change the suggested name.
    bool ok;
    QString what = ( estimates ? "estimates" : "kriging variances" );
    QString new_var_name = QInputDialog::getText(this, "Name the " + what + " variable",
                                             "New variable name:", QLineEdit::Normal,
                                             proposed_name, &ok);
    if (ok && !new_var_name.isEmpty()){
        //the estimates are normally the first variable in the resulting grid
        Attribute* values = m_cg_estimation->getAttributeFromGEOEASIndex( ( estimates ? 1 : 2 ) );
        //add the estimates or variances to the selected estimation grid
        estimation_grid->addGEOEASColumn( values, new_var_name );
    }
}

void FactorialKrigingDialog::onSaveEstimates()
{
    this->onSave( true );
}

void FactorialKrigingDialog::onSaveOrUpdateVModel()
{
	if( ! m_cg_estimation ){
        QMessageBox::critical( this, "Error", "Please, run the estimation at least once.");
        return;
    }

    //get the selected variogram model
    VariogramModel* variogram = m_vModelSelector->getSelectedVModel();

    //propose a name for the file
    QString proposed_name;
    if( ! variogram )
        proposed_name = "variogram.vmodel"; //suggest a default name if the user de-selected the variogram model.
    else
        proposed_name = variogram->getName();

    //open the file rename dialog
    bool ok;
    QString new_var_model_name = QInputDialog::getText(this, "Name variogram model file",
                                             "Variogram model file name (existing name = overwrites):", QLineEdit::Normal,
                                             proposed_name, &ok);
    if (ok && !new_var_model_name.isEmpty()){
        Project *project = Application::instance()->getProject();
        if( project->fileExists( new_var_model_name ) ){
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question( this, "Confirm operation", "Overwrite " + variogram->getName() + "?", \
                                           QMessageBox::Yes | QMessageBox::No );
            if( reply == QMessageBox::Yes ){
                //Overwrites the existing variogram model file (vmodel parameter file)
                QString var_model_file_path = variogram->getPath();
				//m_gpf_kt3d->saveVariogramModel( var_model_file_path );
            }
        } else { //save a new variogram model
            //Generate the parameter file in the tmp directory as if we were about to run vmodel
            QString var_model_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
			//m_gpf_kt3d->saveVariogramModel( var_model_file_path );
            //import the vmodel parameter file as a new variogram model object.
            project->importVariogramModel( var_model_file_path, new_var_model_name );
            //update the variogram model combobox
            m_vModelSelector->updateList();
            //change variogram selection to the newly saved model.
            m_vModelSelector->selectVariogram( new_var_model_name );
        }
    }
}

void FactorialKrigingDialog::onKt3dCompletes()
{
    //frees all signal connections to the GSLib singleton.
    GSLib::instance()->disconnect();

    preview();
}

void FactorialKrigingDialog::onVariogramChanged()
{
	if( ! m_cg_estimation )
        return;
    //get the newly selected variogram model
    VariogramModel* variogram = m_vModelSelector->getSelectedVModel();
    if( ! variogram ){
        return;
    }
    //read the variogram paramters of the newly selected variogram model.
    updateVariogramParameters( variogram );
	Application::instance()->logInfo("NOTE: The user selected a variogram model. Re-reading the variogram parameters.");
}

void FactorialKrigingDialog::onDataSetSelected(DataFile * dataFile)
{
	if( dataFile ){
		if( dataFile->getFileType() == "CARTESIANGRID" )
			ui->frmGrid->hide();
		else
			ui->frmGrid->show();
	} else
		ui->frmGrid->hide();
}

void FactorialKrigingDialog::preview()
{
    if( m_cg_estimation )
        delete m_cg_estimation;

    //get the tmp file path created by kte3d with the estimates and kriging variances
	QString grid_file_path = "NOFILE";

    //create a new grid object corresponding to the file created by kt3d
    m_cg_estimation = new CartesianGrid( grid_file_path );

    //set the grid geometry info.
	//m_cg_estimation->setInfoFromGridParameter( m_gpf_kt3d->getParameter<GSLibParGrid*>(9) );

    //kt3d usually uses -999 as no-data-value.
    m_cg_estimation->setNoDataValue( "-999" );

    //get the variable with the estimation values (normally the first)
    Attribute* est_var = (Attribute*)m_cg_estimation->getChildByIndex( 0 );

    //open the plot dialog
    Util::viewGrid( est_var, this );
}

void FactorialKrigingDialog::updateVariogramParameters(VariogramModel *vm)
{
    //set the variogram number of structures and nugget effect variance contribution
	GSLibParMultiValuedFixed *par20 = nullptr;
    par20->getParameter<GSLibParUInt*>(0)->_value = vm->getNst(); //nst
    par20->getParameter<GSLibParDouble*>(1)->_value = vm->getNugget(); //nugget effect contribution

    //make the necessary copies of variogram structures
	GSLibParRepeat *par21 = nullptr; //repeat nst-times
    par21->setCount( par20->getParameter<GSLibParUInt*>(0)->_value );

    //set each variogram structure parameters
    for( uint ist = 0; ist < par20->getParameter<GSLibParUInt*>(0)->_value; ++ist)
    {
        GSLibParMultiValuedFixed *par21_0 = par21->getParameter<GSLibParMultiValuedFixed*>(ist, 0);
        par21_0->getParameter<GSLibParOption*>(0)->_selected_value = (uint)vm->getIt( ist );
        par21_0->getParameter<GSLibParDouble*>(1)->_value = vm->getCC( ist );
        par21_0->getParameter<GSLibParDouble*>(2)->_value = vm->getAzimuth( ist );
        par21_0->getParameter<GSLibParDouble*>(3)->_value = vm->getDip( ist );
        par21_0->getParameter<GSLibParDouble*>(4)->_value = vm->getRoll( ist );
        GSLibParMultiValuedFixed *par21_1 = par21->getParameter<GSLibParMultiValuedFixed*>(ist, 1);
        par21_1->getParameter<GSLibParDouble*>(0)->_value = vm->get_a_hMax( ist );
        par21_1->getParameter<GSLibParDouble*>(1)->_value = vm->get_a_hMin( ist );
        par21_1->getParameter<GSLibParDouble*>(2)->_value = vm->get_a_vert( ist );
    }
}

void FactorialKrigingDialog::doFK()
{
    // Define the estimation grid.
    {
        DataFile* input_data = static_cast<DataFile*>( this->m_dataSetSelector->getSelectedFile() );
        //the estimation grid depends on input data type.

        if( input_data->getFileType() == "CARTESIANGRID")
            //If the input data is a grid, the estimation grid is itself.
            m_cg_estimation = static_cast<CartesianGrid*>( input_data );
        else
            //If the input data is a pointset, the estimation grid is another grid selected by the user.
            m_cg_estimation = static_cast<CartesianGrid*>( m_cgSelector->getSelectedDataFile() );
    }

    //Build the search strategy object
    SearchStrategy searchStrategy;

    //run the estimation
    {
        FKEstimation estimation;
        estimation.setSearchStrategy( &searchStrategy );
        estimation.setVariogramModel( m_vModelSelector->getSelectedVModel() );
        GSLibParOption* ktype_par = m_gpfFK->getParameter<GSLibParOption*>( 0 );
        estimation.setKrigingType( static_cast<KrigingType>( ktype_par->_selected_value ) );
        GSLibParDouble* skmean_par = m_gpfFK->getParameter<GSLibParDouble*>( 1 );
        estimation.setMeanForSimpleKriging( skmean_par->_value );
        estimation.setInputVariable( m_DataSetVariableSelector->getSelectedVariable() );
        estimation.setEstimationGrid( m_cg_estimation );
        std::vector<double> results = estimation.run();
    }

}
