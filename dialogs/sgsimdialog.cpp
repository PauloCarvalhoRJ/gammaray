#include "sgsimdialog.h"
#include "ui_sgsimdialog.h"
#include "domain/application.h"
#include "domain/variogrammodel.h"
#include "domain/project.h"
#include "domain/pointset.h"
#include "domain/univariatedistribution.h"
#include "domain/cartesiangrid.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "gslib/gslibparams/widgets/widgetgslibpargrid.h"
#include "gslib/gslibparametersdialog.h"
#include "gslib/gslib.h"
#include "widgets/cartesiangridselector.h"
#include "widgets/pointsetselector.h"
#include "widgets/variableselector.h"
#include "widgets/univariatedistributionselector.h"
#include "widgets/variogrammodelselector.h"
#include "widgets/distributionfieldselector.h"
#include "util.h"

#include <QInputDialog>
#include <QMessageBox>

SGSIMDialog::SGSIMDialog( QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SGSIMDialog),
    m_gpf_sgsim( nullptr ),
    m_cg_simulation( nullptr )
{
    ui->setupUi(this);

    this->setWindowTitle("Sequential Gaussian Simulation - SGSIM");

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    //The grid parameters widget (reusing the one orginially made for the GSLib parameter dialog).
    m_par = new GSLibParGrid("", "", "");
    m_par->_specs_x->getParameter<GSLibParUInt*>(0)->_value = 100; //nx
    m_par->_specs_x->getParameter<GSLibParDouble*>(1)->_value = 0; //min x
    m_par->_specs_x->getParameter<GSLibParDouble*>(2)->_value = 1; //cell size x
    m_par->_specs_y->getParameter<GSLibParUInt*>(0)->_value = 100; //ny
    m_par->_specs_y->getParameter<GSLibParDouble*>(1)->_value = 0; //min y
    m_par->_specs_y->getParameter<GSLibParDouble*>(2)->_value = 1; //cell size y
    m_par->_specs_z->getParameter<GSLibParUInt*>(0)->_value = 1; //nz
    m_par->_specs_z->getParameter<GSLibParDouble*>(1)->_value = 0; //min z
    m_par->_specs_z->getParameter<GSLibParDouble*>(2)->_value = 1; //cell size z
    m_gridParameters = new WidgetGSLibParGrid();
    ui->frmGridGeomPlaceholder->layout()->addWidget( m_gridParameters );
    m_gridParameters->fillFields( m_par );

    //Point set widgets
    m_primVarPSetSelector = new PointSetSelector();
    ui->frmPointSetPlaceholder->layout()->addWidget( m_primVarPSetSelector );
    m_primVarSelector = new VariableSelector();
    ui->frmPrimVarPlaceholder->layout()->addWidget( m_primVarSelector );
    m_primVarWgtSelector = new VariableSelector( true );
    ui->frmPrimVarWgtPlaceholder->layout()->addWidget( m_primVarWgtSelector );
    m_primVarSecVarSelector = new VariableSelector( true );
    ui->frmSecVarPlaceholder->layout()->addWidget( m_primVarSecVarSelector );
    connect( m_primVarPSetSelector, SIGNAL(pointSetSelected(DataFile*)),
             m_primVarSelector, SLOT(onListVariables(DataFile*)) );
    connect( m_primVarPSetSelector, SIGNAL(pointSetSelected(DataFile*)),
             m_primVarWgtSelector, SLOT(onListVariables(DataFile*)) );
    connect( m_primVarPSetSelector, SIGNAL(pointSetSelected(DataFile*)),
             m_primVarSecVarSelector, SLOT(onListVariables(DataFile*)) );
    m_primVarPSetSelector->onSelection( 0 );

    //Data transform widgets
    m_refDistFileSelector = new UnivariateDistributionSelector( true );
    ui->frmRefDistFilePlaceholder->layout()->addWidget( m_refDistFileSelector );
    m_refDistValuesSelector = new DistributionFieldSelector( Roles::DistributionColumnRole::VALUE );
    ui->frmDistValuesPlaceholder->layout()->addWidget( m_refDistValuesSelector );
    m_refDistFreqSelector = new DistributionFieldSelector( Roles::DistributionColumnRole::PVALUE );
    ui->frmDistFreqPlaceholder->layout()->addWidget( m_refDistFreqSelector );
    connect( m_refDistFileSelector, SIGNAL(distributionSelected(Distribution*)),
             m_refDistValuesSelector, SLOT(onListFields(Distribution*)) );
    connect( m_refDistFileSelector, SIGNAL(distributionSelected(Distribution*)),
             m_refDistFreqSelector, SLOT(onListFields(Distribution*)) );

    //Grid geometry widgets
    m_gridCopySpecsSelector = new CartesianGridSelector( true );
    ui->frmCopyGridSpecsPlaceholder->layout()->addWidget( m_gridCopySpecsSelector );
    connect( m_gridCopySpecsSelector, SIGNAL(cartesianGridSelected(DataFile*)),
             this, SLOT(onGridCopySpectsSelected(DataFile*)));

    //Secondary data widgets
    m_secVarGridSelector = new CartesianGridSelector( true );
    ui->frmSecDataGridPlaceholder->layout()->addWidget( m_secVarGridSelector );
    m_secVarVariableSelector = new VariableSelector( true );
    ui->frmSecVarSelectorPlaceholder->layout()->addWidget( m_secVarVariableSelector );
    connect( m_secVarGridSelector, SIGNAL(cartesianGridSelected(DataFile*)),
             m_secVarVariableSelector, SLOT(onListVariables(DataFile*)));

    //Variogram model widgets
    m_vModelSelector = new VariogramModelSelector();
    ui->frmVariogramModelPlaceholder->layout()->addWidget( m_vModelSelector );

}

SGSIMDialog::~SGSIMDialog()
{
    delete ui;
    Application::instance()->logInfo("SGSIMDialog destroyed.");
}

void SGSIMDialog::updateVariogramParameters(VariogramModel *vm)
{
    GSLibParVModel *par28 = m_gpf_sgsim->getParameter<GSLibParVModel*>(28);

    //get the sill of the variogram
    //sgsim expects the variogram to be with sill == 1.0, so it is necessary
    //to re-scale variogram parameters.
    double sill = vm->getSill();

    par28->_nst_and_nugget->getParameter<GSLibParUInt*>(0)->_value = vm->getNst(); //nst
    par28->_nst_and_nugget->getParameter<GSLibParDouble*>(1)->_value = vm->getNugget() / sill; //nugget effect contribution (standardized)

    //make the necessary copies of variogram structures
    par28->_variogram_structures->setCount( vm->getNst() ); //repeat nst-times

    //set each variogram structure parameters
    for( uint ist = 0; ist < vm->getNst(); ++ist)
    {
        GSLibParMultiValuedFixed *par28_0 = par28->_variogram_structures->getParameter<GSLibParMultiValuedFixed*>(ist, 0);
        par28_0->getParameter<GSLibParOption*>(0)->_selected_value = (uint)vm->getIt( ist );
        par28_0->getParameter<GSLibParDouble*>(1)->_value = vm->getCC( ist ) / sill; //standardize contributions
        par28_0->getParameter<GSLibParDouble*>(2)->_value = vm->getAzimuth( ist );
        par28_0->getParameter<GSLibParDouble*>(3)->_value = vm->getDip( ist );
        par28_0->getParameter<GSLibParDouble*>(4)->_value = vm->getRoll( ist );
        GSLibParMultiValuedFixed *par28_1 = par28->_variogram_structures->getParameter<GSLibParMultiValuedFixed*>(ist, 1);
        par28_1->getParameter<GSLibParDouble*>(0)->_value = vm->get_a_hMax( ist );
        par28_1->getParameter<GSLibParDouble*>(1)->_value = vm->get_a_hMin( ist );
        par28_1->getParameter<GSLibParDouble*>(2)->_value = vm->get_a_vert( ist );
    }
}

void SGSIMDialog::preview()
{
    if( m_cg_simulation )
        delete m_cg_simulation;

    //get the tmp file path created by sgsim with the realizations
    QString grid_file_path = m_gpf_sgsim->getParameter<GSLibParFile*>(13)->_path;

    //create a new grid object corresponding to the file created by sgsim
    m_cg_simulation = new CartesianGrid( grid_file_path );

    //set the grid geometry info.
    m_cg_simulation->setInfoFromGridParameter( m_gpf_sgsim->getParameter<GSLibParGrid*>(15) );

    //sgsim usually uses -99 as no-data-value.
    m_cg_simulation->setNoDataValue( "-99" );

    //the number of realizations.
    m_cg_simulation->setNReal( m_gpf_sgsim->getParameter<GSLibParUInt*>(14)->_value );

    //get the variable with the simulated values (normally the first)
    Attribute* est_var = (Attribute*)m_cg_simulation->getChildByIndex( 0 );

    //open the plot dialog
    Util::viewGrid( est_var, this );

    //enable the action buttons that depend on completion of sgsim.
    ui->btnRealizationHistogram->setEnabled( true );
    ui->btnEnsembleHistogram->setEnabled( true );
    ui->btnEnsembleVariogram->setEnabled( true );
    ui->btnSaveRealizations->setEnabled( true );
}

void SGSIMDialog::onGridCopySpectsSelected(DataFile *grid)
{
    if( ! grid )
        return;
    m_par->setFromCG((CartesianGrid*)grid);
    m_gridParameters->fillFields( m_par );
}

void SGSIMDialog::onConfigAndRun()
{
    //------------------------Gather simulation data---------------------------------------
    //get the selected input file
    PointSet* input_data_file = (PointSet*)m_primVarPSetSelector->getSelectedDataFile();
    if( ! input_data_file ){
        QMessageBox::critical( this, "Error", "Please, select a point set data file.");
        return;
    }
    input_data_file->loadData();

    //get min and max of variable
    double data_min = input_data_file->min( m_primVarSelector->getSelectedVariableGEOEASIndex()-1 );
    double data_max = input_data_file->max( m_primVarSelector->getSelectedVariableGEOEASIndex()-1 );
    data_min -= fabs( data_min/100.0 );
    data_max += fabs( data_max/100.0 );

    //get the selected variogram model
    VariogramModel* variogram = m_vModelSelector->getSelectedVModel();
    if( ! variogram ){
        QMessageBox::critical( this, "Error", "Please, select a variogram model.");
        return;
    }

    //get the selected grid with secondary data (if any)
    CartesianGrid* sec_data_grid = (CartesianGrid*)m_secVarGridSelector->getSelectedDataFile();

    //-----------------------------set sgsim parameters---------------------------

    if( ! m_gpf_sgsim ){
        //create the parameters object
        m_gpf_sgsim = new GSLibParameterFile("sgsim");

        //set the default values, so we need to set fewer parameters here
        m_gpf_sgsim->setDefaultValues();

        //   zmin,zmax(tail extrapolation)
        GSLibParMultiValuedFixed* par8 = m_gpf_sgsim->getParameter<GSLibParMultiValuedFixed*>(8);
        par8->getParameter<GSLibParDouble*>(0)->_value = data_min;
        par8->getParameter<GSLibParDouble*>(1)->_value = data_max;

        //set the outpt file
        m_gpf_sgsim->getParameter<GSLibParFile*>(13)->_path =
                Application::instance()->getProject()->generateUniqueTmpFilePath("out");

        //initially, set the sample search ellipsoid parameters equivalent to that of the variogram model's
        //anisotropy ellipsoid of the first structure.  It is not necessary to search for uncorrelated samples
        // beyond the variogram ranges.  Of course, the user may change it to better fit one's objectives.
        GSLibParMultiValuedFixed *par22 = m_gpf_sgsim->getParameter<GSLibParMultiValuedFixed*>(22);
        par22->getParameter<GSLibParDouble*>(0)->_value = variogram->get_a_hMax( 0 ); //semi-major axis
        par22->getParameter<GSLibParDouble*>(1)->_value = variogram->get_a_hMin( 0 ); //semi-minor axis
        par22->getParameter<GSLibParDouble*>(2)->_value = variogram->get_a_vert( 0 ); //vertical semi-axis
        uint nst = variogram->getNst();
        for( uint ist = 1; ist < nst; ++ist ){ //sets the initial search radii to the longest range
            double current_hMax = variogram->get_a_hMax( ist );
            double current_hMin = variogram->get_a_hMin( ist );
            double current_vert = variogram->get_a_vert( ist );
            if( par22->getParameter<GSLibParDouble*>(0)->_value < current_hMax )
                par22->getParameter<GSLibParDouble*>(0)->_value = current_hMax;
            if( par22->getParameter<GSLibParDouble*>(1)->_value < current_hMin )
                par22->getParameter<GSLibParDouble*>(1)->_value = current_hMin;
            if( par22->getParameter<GSLibParDouble*>(2)->_value < current_vert )
                par22->getParameter<GSLibParDouble*>(2)->_value = current_vert;
        }
        GSLibParMultiValuedFixed *par23 = m_gpf_sgsim->getParameter<GSLibParMultiValuedFixed*>(23);
        par23->getParameter<GSLibParDouble*>(0)->_value = variogram->getAzimuth( 0 ); //azimuth
        par23->getParameter<GSLibParDouble*>(1)->_value = variogram->getDip( 0 ); //dip
        par23->getParameter<GSLibParDouble*>(2)->_value = variogram->getRoll( 0 ); //roll

        //read the variogram parameters
        updateVariogramParameters( variogram );
    }

    //-------The parameters from this point on are reset for each run------------------/

    //file with data
    m_gpf_sgsim->getParameter<GSLibParFile*>(0)->_path = input_data_file->getPath();
    //   columns for X,Y,Z,vr,wt,sec.var.
    GSLibParMultiValuedFixed* par1 = m_gpf_sgsim->getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = input_data_file->getXindex();
    par1->getParameter<GSLibParUInt*>(1)->_value = input_data_file->getYindex();
    par1->getParameter<GSLibParUInt*>(2)->_value = input_data_file->getZindex();
    par1->getParameter<GSLibParUInt*>(3)->_value = m_primVarSelector->getSelectedVariableGEOEASIndex();
    par1->getParameter<GSLibParUInt*>(4)->_value = m_primVarWgtSelector->getSelectedVariableGEOEASIndex();
    par1->getParameter<GSLibParUInt*>(5)->_value = m_primVarSecVarSelector->getSelectedVariableGEOEASIndex();
    //  trimming limits
    GSLibParMultiValuedFixed* par2 = m_gpf_sgsim->getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParDouble*>(0)->_value = data_min;
    par2->getParameter<GSLibParDouble*>(1)->_value = data_max;
    //transform the data (0=no, 1=yes)
    m_gpf_sgsim->getParameter<GSLibParOption*>(3)->_selected_value = ui->chkEnableTransform->isChecked()?1:0;
    //   file for output trans table
    m_gpf_sgsim->getParameter<GSLibParFile*>(4)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("trn");
    //   consider ref. dist (0=no, 1=yes)
    m_gpf_sgsim->getParameter<GSLibParOption*>(5)->_selected_value = m_refDistFileSelector->getSelectedDistribution()?1:0;
    //   file with ref. dist distribution
    if( m_refDistFileSelector->getSelectedDistribution() )
        m_gpf_sgsim->getParameter<GSLibParFile*>(6)->_path = m_refDistFileSelector->getSelectedDistribution()->getPath();
    else
        m_gpf_sgsim->getParameter<GSLibParFile*>(6)->_path = "nofile.dst";
    //    columns for vr and wt
    GSLibParMultiValuedFixed* par7 = m_gpf_sgsim->getParameter<GSLibParMultiValuedFixed*>(7);
    par7->getParameter<GSLibParUInt*>(0)->_value = m_refDistValuesSelector->getSelectedFieldGEOEASIndex();
    par7->getParameter<GSLibParUInt*>(1)->_value = m_refDistFreqSelector->getSelectedFieldGEOEASIndex();
    //nx,xmn,xsiz; ny,ymn,ysiz; nz,zmn,zsiz
    m_gridParameters->updateValue( m_par ); //read values from widgets onto the grid parameter object
    GSLibParGrid* par15 = m_gpf_sgsim->getParameter<GSLibParGrid*>(15);
    par15->_specs_x->getParameter<GSLibParUInt*>(0)->_value = m_par->getNX(); //nx
    par15->_specs_x->getParameter<GSLibParDouble*>(1)->_value = m_par->getX0(); //min x
    par15->_specs_x->getParameter<GSLibParDouble*>(2)->_value = m_par->getDX(); //cell size x
    par15->_specs_y->getParameter<GSLibParUInt*>(0)->_value = m_par->getNY(); //ny
    par15->_specs_y->getParameter<GSLibParDouble*>(1)->_value = m_par->getY0(); //min y
    par15->_specs_y->getParameter<GSLibParDouble*>(2)->_value = m_par->getDY(); //cell size y
    par15->_specs_z->getParameter<GSLibParUInt*>(0)->_value = m_par->getNZ(); //nz
    par15->_specs_z->getParameter<GSLibParDouble*>(1)->_value = m_par->getZ0(); //min z
    par15->_specs_z->getParameter<GSLibParDouble*>(2)->_value = m_par->getDZ(); //cell size z
    //   file with LVM, EXDR, or COLC variable
    if( m_secVarGridSelector->getSelectedDataFile() )
        m_gpf_sgsim->getParameter<GSLibParFile*>(26)->_path = m_secVarGridSelector->getSelectedDataFile()->getPath();
    else
        m_gpf_sgsim->getParameter<GSLibParFile*>(26)->_path = "nofile.dat";
    //   column for secondary variable
    m_gpf_sgsim->getParameter<GSLibParUInt*>(27)->_value = m_secVarVariableSelector->getSelectedVariableGEOEASIndex();

    //----------------------------prepare and execute sgsim--------------------------------
    //show the sgsim parameters
    GSLibParametersDialog gsd( m_gpf_sgsim, this );
    int result = gsd.exec();

    //if user didn't cancel the dialog
    if( result == QDialog::Accepted ){
        //Generate the parameter file
        QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "par" );
        m_gpf_sgsim->save( par_file_path );

        //to be notified when sgsim completes.
        connect( GSLib::instance(), SIGNAL(programFinished()), this, SLOT(onSgsimCompletes()) );

        //run sgsim program asynchronously
        Application::instance()->logInfo("Starting sgsim program...");
        GSLib::instance()->runProgramAsync( "sgsim", par_file_path );
    }

}

void SGSIMDialog::onVariogramChanged()
{
    if( ! m_gpf_sgsim )
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

void SGSIMDialog::onSgsimCompletes()
{
    //frees all signal connections to the GSLib singleton.
    GSLib::instance()->disconnect();

    preview();
}

void SGSIMDialog::onRealizationHistogram()
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
    GSLibParameterFile gpf( "addcoord" );

    //input file
    GSLibParFile* par0 = gpf.getParameter<GSLibParFile*>( 0 );
    par0->_path = cg->getPath();

    //output file (temporary)
    GSLibParFile* par1 = gpf.getParameter<GSLibParFile*>( 1 );
    par1->_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "xyz" );

    //realization number
    GSLibParUInt* par2 = gpf.getParameter<GSLibParUInt*>( 2 );
    par2->_value = realNumber;

    //grid parameters
    GSLibParGrid* par3 = gpf.getParameter<GSLibParGrid*>( 3 );
    par3->setFromCG( cg );

    //Generate the parameter file
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    gpf.save( par_file_path );

    //run addcoord program
    Application::instance()->logInfo("Starting addcoord program...");
    GSLib::instance()->runProgram( "addcoord", par_file_path );
    Application::instance()->logInfo("addcoord completed.");

    //make the point set object from the temporary point set file created by addcoord
    PointSet *ps = new PointSet( par1->_path );

    //addcoord always adds the X,Y,Z fields as 1st, 2nd and 3rd variables in the data file
    //the ndv value is the same as the original Cartesian grid.
    ps->setInfo( 1, 2, 3, cg->getNoDataValue() );

    //the simulated values of the target realization is the 4th GEO-EAS column in the point set file
    //generated by addcord
    Attribute *at = ps->getAttributeFromGEOEASIndex( 4 );
    Util::viewHistogram( at, this );
}

void SGSIMDialog::onEnsembleHistogram()
{

}

void SGSIMDialog::onEnsembleVariogram()
{

}

void SGSIMDialog::onSaveEnsemble()
{

}
