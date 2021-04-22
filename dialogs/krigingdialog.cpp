#include "krigingdialog.h"
#include "ui_krigingdialog.h"
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
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslibparametersdialog.h"
#include "gslib/gslib.h"
#include "util.h"

#include <QInputDialog>
#include <QMessageBox>
#include <cmath>

KrigingDialog::KrigingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::KrigingDialog),
    m_gpf_kt3d( nullptr ),
    m_cg_estimation( nullptr )
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

    //The list with existing cartesian grids in the project for the secondary data.
    m_cgSelectorSecondary = new CartesianGridSelector( true );
    ui->frmSecondaryData->layout()->addWidget( m_cgSelectorSecondary );

    //The list with the secondary data grid variables;
    m_cgSecondaryVariableSelector = new VariableSelector();
    ui->frmSecondaryData->layout()->addWidget( m_cgSecondaryVariableSelector );
    connect( m_cgSelectorSecondary, SIGNAL(cartesianGridSelected(DataFile*)),
             m_cgSecondaryVariableSelector, SLOT(onListVariables(DataFile*)) );

    //The list with existing point sets in the project.
    m_psSelector = new PointSetSelector();
    ui->frmData->layout()->addWidget( m_psSelector );

    //The list with the Point Set variables to set the primary variable
    m_PointSetVariableSelector = new VariableSelector();
    ui->frmData->layout()->addWidget( m_PointSetVariableSelector );
    connect( m_psSelector, SIGNAL(pointSetSelected(DataFile*)),
             m_PointSetVariableSelector, SLOT(onListVariables(DataFile*)) );

    //The list with the Point Set variables to set the secondary variable
    m_PointSetSecondaryVariableSelector = new VariableSelector( true );
    ui->frmData->layout()->addWidget( m_PointSetSecondaryVariableSelector );
    connect( m_psSelector, SIGNAL(pointSetSelected(DataFile*)),
             m_PointSetSecondaryVariableSelector, SLOT(onListVariables(DataFile*)) );

    //calling this slot causes the variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_psSelector->onSelection( 0 );

     if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI ){
        ui->btnParameters->setIcon( QIcon(":icons32/setting32") );
        ui->btnSaveKVariances->setIcon( QIcon(":icons32/save32") );
        ui->btnSave->setIcon( QIcon(":icons32/save32") );
        ui->btnSaveOrUpdateVModel->setIcon( QIcon(":icons32/save32") );
        ui->btnXValidation->setIcon( QIcon(":icons32/xplot32") );
    }


    adjustSize();
}

KrigingDialog::~KrigingDialog()
{
    delete ui;
    if( m_gpf_kt3d )
        delete m_gpf_kt3d;
    Application::instance()->logInfo("KrigingDialog destroyed.");
}

void KrigingDialog::onParameters()
{

    //-------------------------gather estimation info-----------------------------

    //get the selected input file
    PointSet* input_data_file = (PointSet*)m_psSelector->getSelectedDataFile();
    if( ! input_data_file ){
        QMessageBox::critical( this, "Error", "Please, select a point set data file.");
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
    double data_min = input_data_file->min( m_PointSetVariableSelector->getSelectedVariableGEOEASIndex()-1 );
    double data_max = input_data_file->max( m_PointSetVariableSelector->getSelectedVariableGEOEASIndex()-1 );
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

    //-----------------------------set kt3d parameters---------------------------

    if( ! m_gpf_kt3d ){
        //create the parameters object
        m_gpf_kt3d = new GSLibParameterFile("kt3d");

        //set the default values, so we need to set fewer parameters here
        m_gpf_kt3d->setDefaultValues();

        //set the outpt file
        m_gpf_kt3d->getParameter<GSLibParFile*>(8)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("out");

        //initially, set the sample search ellipsoid parameters equivalent to that of the variogram model's
        //anisotropy ellipsoid of the first structure.  It is not necessary to search for uncorrelated samples
        // beyond the variogram ranges.  Of course, the user may change it to better fit one's objectives.
        GSLibParMultiValuedFixed *par13 = m_gpf_kt3d->getParameter<GSLibParMultiValuedFixed*>(13);
        par13->getParameter<GSLibParDouble*>(0)->_value = variogram->get_a_hMax( 0 ); //semi-major axis
        par13->getParameter<GSLibParDouble*>(1)->_value = variogram->get_a_hMin( 0 ); //semi-minor axis
        par13->getParameter<GSLibParDouble*>(2)->_value = variogram->get_a_vert( 0 ); //vertical semi-axis
        uint nst = variogram->getNst();
        for( uint ist = 1; ist < nst; ++ist ){ //sets the initial search radii to the longest range
            double current_hMax = variogram->get_a_hMax( ist );
            double current_hMin = variogram->get_a_hMin( ist );
            double current_vert = variogram->get_a_vert( ist );
            if( par13->getParameter<GSLibParDouble*>(0)->_value < current_hMax )
                par13->getParameter<GSLibParDouble*>(0)->_value = current_hMax;
            if( par13->getParameter<GSLibParDouble*>(1)->_value < current_hMin )
                par13->getParameter<GSLibParDouble*>(1)->_value = current_hMin;
            if( par13->getParameter<GSLibParDouble*>(2)->_value < current_vert )
                par13->getParameter<GSLibParDouble*>(2)->_value = current_vert;
        }
        GSLibParMultiValuedFixed *par14 = m_gpf_kt3d->getParameter<GSLibParMultiValuedFixed*>(14);
        par14->getParameter<GSLibParDouble*>(0)->_value = variogram->getAzimuth( 0 ); //azimuth
        par14->getParameter<GSLibParDouble*>(1)->_value = variogram->getDip( 0 ); //dip
        par14->getParameter<GSLibParDouble*>(2)->_value = variogram->getRoll( 0 ); //roll

        //read the variogram parameters
        updateVariogramParameters( variogram );
    }

    //the setting from this line on are reset to the values from the selected files in each run

    //set the input data file
    m_gpf_kt3d->getParameter<GSLibParFile*>(0)->_path = input_data_file->getPath();

    //set the GEO-EAS column indexes
    GSLibParMultiValuedFixed *par1 = m_gpf_kt3d->getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = 0; //drillhole ID is not used yet
    par1->getParameter<GSLibParUInt*>(1)->_value = input_data_file->getXindex(); //X
    par1->getParameter<GSLibParUInt*>(2)->_value = input_data_file->getYindex(); //Y
    par1->getParameter<GSLibParUInt*>(3)->_value = input_data_file->getZindex(); //Z
    par1->getParameter<GSLibParUInt*>(4)->_value = \
            m_PointSetVariableSelector->getSelectedVariableGEOEASIndex(); //variable
    par1->getParameter<GSLibParUInt*>(5)->_value = \
            m_PointSetSecondaryVariableSelector->getSelectedVariableGEOEASIndex(); //sec. variable

    //set the trimming limits
    GSLibParMultiValuedFixed *par2 = m_gpf_kt3d->getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParDouble*>(0)->_value = data_min;
    par2->getParameter<GSLibParDouble*>(1)->_value = data_max;

    //set the kriging mode ( 0 = estimate over grid )
    m_gpf_kt3d->getParameter<GSLibParOption*>(3)->_selected_value = 0;

    //set the grid file with secondary data
    if( sec_data_grid )
        m_gpf_kt3d->getParameter<GSLibParFile*>(18)->_path = sec_data_grid->getPath();
    else
        m_gpf_kt3d->getParameter<GSLibParFile*>(18)->_path = "nofile.dat";

    //set the selected column in the secondary data grid
    m_gpf_kt3d->getParameter<GSLibParUInt*>(19)->_value = m_cgSecondaryVariableSelector->getSelectedVariableGEOEASIndex();

    //set the grid parameters
    GSLibParGrid* par9;
    par9 = m_gpf_kt3d->getParameter<GSLibParGrid*>(9);
    par9->_specs_x->getParameter<GSLibParUInt*>(0)->_value = estimation_grid->getNX(); //nx
    par9->_specs_x->getParameter<GSLibParDouble*>(1)->_value = estimation_grid->getX0(); //min x
    par9->_specs_x->getParameter<GSLibParDouble*>(2)->_value = estimation_grid->getDX(); //cell size x
    par9->_specs_y->getParameter<GSLibParUInt*>(0)->_value = estimation_grid->getNY(); //ny
    par9->_specs_y->getParameter<GSLibParDouble*>(1)->_value = estimation_grid->getY0(); //min y
    par9->_specs_y->getParameter<GSLibParDouble*>(2)->_value = estimation_grid->getDY(); //cell size y
    par9->_specs_z->getParameter<GSLibParUInt*>(0)->_value = estimation_grid->getNZ(); //nz
    par9->_specs_z->getParameter<GSLibParDouble*>(1)->_value = estimation_grid->getZ0(); //min z
    par9->_specs_z->getParameter<GSLibParDouble*>(2)->_value = estimation_grid->getDZ(); //cell size z

    //----------------------------prepare and execute kt3d--------------------------------

    //show the kt3d parameters
    GSLibParametersDialog gsd( m_gpf_kt3d, this );
    int result = gsd.exec();

    //if user didn't cancel the dialog
    if( result == QDialog::Accepted ){
        //Generate the parameter file
        QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "par" );
        m_gpf_kt3d->save( par_file_path );

        //to be notified when kt3d completes.
        connect( GSLib::instance(), SIGNAL(programFinished()), this, SLOT(onKt3dCompletes()) );

        //run kt3d program asynchronously
        Application::instance()->logInfo("Starting kt3d program...");
        GSLib::instance()->runProgramAsync( "kt3d", par_file_path );
    }
}

void KrigingDialog::onXValidation()
{
    if( ! m_gpf_kt3d ){
        QMessageBox::critical( this, "Error", "Please, run the estimation at least once.");
        return;
    }

    //change only the kriging mode ( 1 = estimate for cross validation )
    m_gpf_kt3d->getParameter<GSLibParOption*>(3)->_selected_value = 1;

    //Generate the parameter file
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "par" );
    m_gpf_kt3d->save( par_file_path );

    //re-run kt3d program
    Application::instance()->logInfo("Starting kt3d program...");
    GSLib::instance()->runProgram( "kt3d", par_file_path );

    //get the tmp file path created by kt3d with the sample values and estimates
    QString estimation_file_path = m_gpf_kt3d->getParameter<GSLibParFile*>(8)->_path;

    //create a new point set object corresponding to the file created by kt3d
    PointSet* ps = new PointSet( estimation_file_path );

    //expecting kt3d outputs a x-validation point-set with coordinates X, Y, Z (even if the samples are 2D)
    //as columns 1, 2 and 3 and with no-data-values equal to -999
    ps->setInfo( 1, 2, 3, "-999");

    //get the variable with the sample values (normally the 4th)
    Attribute* true_var = (Attribute*)ps->getChildByIndex( 3 );

    //get the variable with the estimation values (normally the 5th)
    Attribute* est_var = (Attribute*)ps->getChildByIndex( 4 );

    //open the plot dialog
    Util::viewXPlot( true_var, est_var, this );

    //TODO: delete the ps object created here in a manner that the dialog doesn't crash.
}

void KrigingDialog::onSave(bool estimates)
{
    //TODO: this onSave() method (or the functions it calls) crashed once.
    if( ! m_gpf_kt3d || ! m_cg_estimation ){
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
    QString proposed_name( m_PointSetVariableSelector->getSelectedVariableName() );
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
		if( values ){
			//add the estimates or variances to the selected estimation grid
			estimation_grid->addGEOEASColumn( values, new_var_name );
		} else {
			QMessageBox::critical( this, "Error", "KrigingDialog::onSave: m_cd_estimation is null. Please, run the estimation again.");
		}
    }
}

void KrigingDialog::onSaveEstimates()
{
    this->onSave( true );
}

void KrigingDialog::onSaveKVariances()
{
    this->onSave( false );
}

void KrigingDialog::onSaveOrUpdateVModel()
{
    if( ! m_gpf_kt3d ){
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
                m_gpf_kt3d->saveVariogramModel( var_model_file_path );
            }
        } else { //save a new variogram model
            //Generate the parameter file in the tmp directory as if we were about to run vmodel
            QString var_model_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
            m_gpf_kt3d->saveVariogramModel( var_model_file_path );
            //import the vmodel parameter file as a new variogram model object.
            project->importVariogramModel( var_model_file_path, new_var_model_name );
            //update the variogram model combobox
            m_vModelSelector->updateList();
            //change variogram selection to the newly saved model.
            m_vModelSelector->selectVariogram( new_var_model_name );
        }
    }
}

void KrigingDialog::onKt3dCompletes()
{
    //frees all signal connections to the GSLib singleton.
    GSLib::instance()->disconnect();

    preview();
}

void KrigingDialog::onVariogramChanged()
{
    if( ! m_gpf_kt3d )
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

void KrigingDialog::preview()
{
    if( m_cg_estimation )
        delete m_cg_estimation;

    //get the tmp file path created by kte3d with the estimates and kriging variances
    QString grid_file_path = m_gpf_kt3d->getParameter<GSLibParFile*>(8)->_path;

    //Sanity check
    QFile grid_file( grid_file_path );
    if( ! grid_file.exists( ) ){
        Application::instance()->logError( "KrigingDialog::preview(): output file not generated.  "
                                           "Check kt3d parameters and the output message pane for errors"
                                           " issued by kt3d.", true );
        return;
    }

    //create a new grid object corresponding to the file created by kt3d
    m_cg_estimation = new CartesianGrid( grid_file_path );

    //set the grid geometry info.
    m_cg_estimation->setInfoFromGridParameter( m_gpf_kt3d->getParameter<GSLibParGrid*>(9) );

    //kt3d usually uses -999 as no-data-value.
    m_cg_estimation->setNoDataValue( "-999" );

    //get the variable with the estimation values (normally the first)
    Attribute* est_var = (Attribute*)m_cg_estimation->getChildByIndex( 0 );

    //open the plot dialog
    Util::viewGrid( est_var, this );
}

void KrigingDialog::updateVariogramParameters(VariogramModel *vm)
{
    //set the variogram number of structures and nugget effect variance contribution
    GSLibParMultiValuedFixed *par20 = m_gpf_kt3d->getParameter<GSLibParMultiValuedFixed*>(20);
    par20->getParameter<GSLibParUInt*>(0)->_value = vm->getNst(); //nst
    par20->getParameter<GSLibParDouble*>(1)->_value = vm->getNugget(); //nugget effect contribution

    //make the necessary copies of variogram structures
    GSLibParRepeat *par21 = m_gpf_kt3d->getParameter<GSLibParRepeat*>(21); //repeat nst-times
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
