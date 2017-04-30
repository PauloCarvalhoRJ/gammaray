#include "variogramanalysisdialog.h"
#include "ui_variogramanalysisdialog.h"
#include "domain/pointset.h"
#include "domain/cartesiangrid.h"
#include "domain/experimentalvariogram.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslib.h"
#include "gslib/gslibparametersdialog.h"
#include "domain/project.h"
#include "domain/attribute.h"
#include "domain/application.h"
#include <QMessageBox>
#include "displayplotdialog.h"
#include <QDir>
#include <QInputDialog>
#include <cmath>
#include <util.h>

#define C_180_OVER_PI (180.0 / 3.14159265)

VariogramAnalysisDialog::VariogramAnalysisDialog(Attribute *head, Attribute *tail, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VariogramAnalysisDialog),
    m_head( head ),
    m_tail( tail ),
    m_ev( nullptr ),
    m_gpf_varmap( nullptr ),
    m_gpf_pixelplt( nullptr ),
    m_gpf_gamv( nullptr ),
    m_gpf_vargplt_exp_irreg( nullptr ),
    m_gpf_gam( nullptr ),
    m_varmap_grid( nullptr ),
    m_gpf_vmodel( nullptr )
{
    ui->setupUi(this);

    ui->lblHead->setText("Head: <font color=\"red\"><b>" + m_head->getName() + "</b></color>" );
    ui->lblTail->setText("Tail: <font color=\"blue\"><b>" + m_tail->getName() + "</b></color>");

    this->setWindowTitle( QString("Variogram analysis on ").append( m_head->getContainingFile()->getName() ) );

    finishUISetup();
}

VariogramAnalysisDialog::VariogramAnalysisDialog(ExperimentalVariogram *ev, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VariogramAnalysisDialog),
    m_head( nullptr ),
    m_tail( nullptr ),
    m_ev( ev ),
    m_gpf_varmap( nullptr ),
    m_gpf_pixelplt( nullptr ),
    m_gpf_gamv( nullptr ),
    m_gpf_vargplt_exp_irreg( nullptr ),
    m_gpf_gam( nullptr ),
    m_varmap_grid( nullptr ),
    m_gpf_vmodel( nullptr )
{
    ui->setupUi(this);
    this->setWindowTitle( QString("Fit variogram model for ").append( ev->getName() ) );
    ui->frmSelection->setVisible( false );
    ui->frmGamGamv->setVisible( false );
    ui->frmVarmap->setVisible( false );
    ui->lbl3->setText( "Variogram model:" );

    finishUISetup();
}

VariogramAnalysisDialog::~VariogramAnalysisDialog()
{
    delete ui;
    if( m_gpf_gamv )
        delete m_gpf_gamv;
    if( m_varmap_grid )
        delete m_varmap_grid;
    if( m_gpf_varmap )
        delete m_gpf_varmap;
    if( m_gpf_pixelplt )
        delete m_gpf_pixelplt;
    if( m_gpf_vargplt_exp_irreg )
        delete m_gpf_vargplt_exp_irreg;
    if( m_gpf_gam )
        delete m_gpf_gam;
    if( m_gpf_vmodel )
        delete m_gpf_vmodel;
    Application::instance()->logInfo("Variogram analysis dialog destroyed.");
}

void VariogramAnalysisDialog::finishUISetup()
{
    if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI ){
        ui->btnExpVarioPar->setIcon( QIcon(":icons32/setting32") );
        ui->btnExpVarioPlot->setIcon( QIcon(":icons32/plot32") );
        ui->btnExpVarioSave->setIcon( QIcon(":icons32/save32") );
        ui->btnModelVarioPar->setIcon( QIcon(":icons32/setting32") );
        ui->btnModelVarioPlot->setIcon( QIcon(":icons32/plot32") );
        ui->btnModelVarioSave->setIcon( QIcon(":icons32/save32") );
        ui->btnVarioMapSave->setIcon( QIcon(":icons32/save32") );
        ui->btnVarmapPar->setIcon( QIcon(":icons32/setting32") );
        ui->btnVarmapPlot->setIcon( QIcon(":icons32/plot32") );
    }

    adjustSize();
}

bool VariogramAnalysisDialog::isCrossVariography()
{
    return m_head != m_tail;
}

void VariogramAnalysisDialog::onOpenVarMapParameters()
{
    if( ! m_gpf_varmap ){
        //Construct an object composition based on the parameter file template for the gamv program.
        m_gpf_varmap = new GSLibParameterFile( "varmap" );
        //Get the data file (can be either point set or grid).
        DataFile* input_data_file = (DataFile*)m_head->getContainingFile();
        //loads data in file.
        input_data_file->loadData();
        //get the variables indexes in parent data file
        uint head_var_index = input_data_file->getFieldGEOEASIndex( m_head->getName() );
        uint tail_var_index = head_var_index;
        if( isCrossVariography() )
             tail_var_index = input_data_file->getFieldGEOEASIndex( m_tail->getName() );
        //Set default values so we need to change less parameters and let
        //the user change the others as one may see fit.
        m_gpf_varmap->setDefaultValues();
        //get the max and min of the selected variables
        double data_min = input_data_file->min( head_var_index-1 );
        double data_max = input_data_file->max( head_var_index-1 );
        if( isCrossVariography() ){
            double tail_min = input_data_file->min( tail_var_index-1 );
            double tail_max = input_data_file->max( tail_var_index-1 );
            if( tail_min < data_min )
                data_min = tail_min;
            if( tail_max > data_max )
                data_max = tail_max;
        }
        //----------------set the minimum required gamv paramaters-----------------------
        //input file
        m_gpf_varmap->getParameter<GSLibParFile*>(0)->_path = input_data_file->getPath();
        //number of variables and column numbers
        GSLibParMultiValuedFixed *par1 = m_gpf_varmap->getParameter<GSLibParMultiValuedFixed*>(1);
        if( isCrossVariography() ){
            par1->getParameter<GSLibParUInt*>(0)->_value = 2;
        }
        GSLibParMultiValuedVariable *par1_1 = par1->getParameter<GSLibParMultiValuedVariable*>(1);
        par1_1->getParameter<GSLibParUInt*>(0)->_value = head_var_index;
        if( isCrossVariography() ){
            par1_1->assure( 2 );
            par1_1->getParameter<GSLibParUInt*>(1)->_value = tail_var_index;
        }
        //data minimum and maximum (trimming limits)
        GSLibParMultiValuedFixed *par2 = m_gpf_varmap->getParameter<GSLibParMultiValuedFixed*>(2);
        par2->getParameter<GSLibParDouble*>(0)->_value = data_min;
        par2->getParameter<GSLibParDouble*>(1)->_value = data_max;
        //determine whether the data is regular (grid) or irregular (point cloud)
        bool is_irregular = (input_data_file->getFileType().compare("POINTSET") == 0);
        //data set type
        m_gpf_varmap->getParameter<GSLibParOption*>(3)->_selected_value = ( is_irregular ? 0 : 1 );
        if( ! is_irregular ){ //if data set is regular
            //cast data file to cartesian grid
            CartesianGrid* cgrid = (CartesianGrid*)input_data_file;
            //grid dimensions
            GSLibParMultiValuedFixed *par4 = m_gpf_varmap->getParameter<GSLibParMultiValuedFixed*>(4);
            par4->getParameter<GSLibParUInt*>(0)->_value = cgrid->getNX();
            par4->getParameter<GSLibParUInt*>(1)->_value = cgrid->getNY();
            par4->getParameter<GSLibParUInt*>(2)->_value = cgrid->getNZ();
            //grid cell size
            GSLibParMultiValuedFixed *par5 = m_gpf_varmap->getParameter<GSLibParMultiValuedFixed*>(5);
            par5->getParameter<GSLibParDouble*>(0)->_value = cgrid->getDX();
            par5->getParameter<GSLibParDouble*>(1)->_value = cgrid->getDY();
            par5->getParameter<GSLibParDouble*>(2)->_value = cgrid->getDZ();
        } else { //if data set is irregular
            //cast data file to point set
            PointSet* pset = (PointSet*)input_data_file;
            //X,Y,Z column indexes
            GSLibParMultiValuedFixed *par6 = m_gpf_varmap->getParameter<GSLibParMultiValuedFixed*>(6);
            par6->getParameter<GSLibParUInt*>(0)->_value = pset->getXindex();
            par6->getParameter<GSLibParUInt*>(1)->_value = pset->getYindex();
            par6->getParameter<GSLibParUInt*>(2)->_value = pset->getZindex();
        }
        //output grid with varmap image
        m_gpf_varmap->getParameter<GSLibParFile*>(7)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("out");

        //define a non-default variogram option if the user opted for cross variography
        if( isCrossVariography() ){
            GSLibParRepeat *par13 = m_gpf_varmap->getParameter<GSLibParRepeat*>(13);
            GSLibParMultiValuedFixed *par13_0 = par13->getParameter<GSLibParMultiValuedFixed*>(0, 0);
            par13_0->getParameter<GSLibParUInt*>(0)->_value = 1;
            par13_0->getParameter<GSLibParUInt*>(1)->_value = 2;
            par13_0->getParameter<GSLibParOption*>(2)->_selected_value = 2;
        }
    }
    //construct the parameter dialog so the user can adjust other settings before running varmap
    GSLibParametersDialog gslibpardiag( m_gpf_varmap );
    int result = gslibpardiag.exec();
    if( result == QDialog::Accepted ){
        //Generate the parameter file
        QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
        m_gpf_varmap->save( par_file_path );
        //to be notified when varmap completes.
        connect( GSLib::instance(), SIGNAL(programFinished()), this, SLOT(onVarmapCompletion()) );
        //run varmap program asynchronously (user can see the program outputs while it runs)
        Application::instance()->logInfo("Starting varmap program...");
        GSLib::instance()->runProgramAsync( "varmap", par_file_path );
    }
}

void VariogramAnalysisDialog::onOpenVarMapPlot()
{
    if( ! m_gpf_varmap ){
        QMessageBox::critical( this, "Error", "You must first compute the variogram map at least once.");
        return;
    } else {
        delete m_varmap_grid;
    }


    //make plot/window title
    QString title = "Variogram map of ";
    if( isCrossVariography() )
        title = "Cross-variogram ";
    title.append( m_head->getName() );
    if( isCrossVariography() )
        title.append( " X " + m_tail->getName() );

    //replace asterisks caused by a bug in varmap with no-data values
    Util::fixVarmapBug( m_gpf_varmap->getParameter<GSLibParFile*>(7)->_path );

    //create a cartesian grid object from the grid generated by the varmap program
    m_varmap_grid = new CartesianGrid( m_gpf_varmap->getParameter<GSLibParFile*>(7)->_path );

    //load the grid data
    m_varmap_grid->loadData();

    //get lag geometry
    GSLibParMultiValuedFixed *par8 = m_gpf_varmap->getParameter<GSLibParMultiValuedFixed*>(8);
    uint nxlags = par8->getParameter<GSLibParUInt*>(0)->_value;
    uint nylags = par8->getParameter<GSLibParUInt*>(1)->_value;
    uint nzlags = par8->getParameter<GSLibParUInt*>(2)->_value;
    GSLibParMultiValuedFixed *par9 = m_gpf_varmap->getParameter<GSLibParMultiValuedFixed*>(9);
    double xlag = par9->getParameter<GSLibParDouble*>(0)->_value;
    double ylag = par9->getParameter<GSLibParDouble*>(1)->_value;
    double zlag = par9->getParameter<GSLibParDouble*>(2)->_value;

    //set grid parameters
    QMap<uint, QPair<uint, QString> > empty;
    QList< QPair<uint,QString> > empty2;
    m_varmap_grid->setInfo(nxlags * -xlag, nylags * -ylag, nzlags * -zlag,
                             xlag,           ylag,           zlag,
                             nxlags*2+1,     nylags*2+1,     nzlags*2+1,
                             0, 1, Util::VARMAP_NDV, empty, empty2 );
    m_varmap_grid->setInfoFromMetadataFile();

    //variogram value is the first variable in the grid file generated by varmap
    uint var_index = 1;

    //get the max and min of variogram value (first index)
    double data_min = m_varmap_grid->min( var_index - 1 );
    double data_max = m_varmap_grid->max( var_index - 1 );

    if( ! m_gpf_pixelplt ) {
        //Construct an object composition based on the parameter file template for the pixelplt program.
        m_gpf_pixelplt = new GSLibParameterFile ( "pixelplt" );

        //Set default values so we need to change less parameters and let
        //the user change the others as one may see fit.
        m_gpf_pixelplt->setDefaultValues();

        //----------------set the minimum required pixelplt paramaters-----------------------
        //input data parameters: data file, trimming limits and var,weight indexes
        GSLibParInputData* par0;
        par0 = m_gpf_pixelplt->getParameter<GSLibParInputData*>(0);
        par0->_file_with_data._path = m_varmap_grid->getPath();
        par0->_var_wgt_pairs.first()->_var_index = var_index;
        par0->_trimming_limits._min = data_min - fabs( data_min/100.0 );
        par0->_trimming_limits._max = data_max + fabs( data_max/100.0 );

        //postscript file
        m_gpf_pixelplt->getParameter<GSLibParFile*>(1)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("ps");

        //plot title
        m_gpf_pixelplt->getParameter<GSLibParString*>(6)->_value = title;

        //horizontal axis (normally X) label
        m_gpf_pixelplt->getParameter<GSLibParString*>(7)->_value = "h";

        //vertical axis (normally Y) label
        m_gpf_pixelplt->getParameter<GSLibParString*>(8)->_value = "h";

        //min, max, resolution for color scale
        GSLibParMultiValuedFixed* par12 = m_gpf_pixelplt->getParameter<GSLibParMultiValuedFixed*>(12);
        par12->getParameter<GSLibParDouble*>(0)->_value = data_min;
        par12->getParameter<GSLibParDouble*>(1)->_value = data_max;
        par12->getParameter<GSLibParDouble*>(2)->_value = (data_max-data_min)/10.0;
    }

    //grid definition parameters (must be redone for each run, discarding what the user might entered)
    GSLibParGrid* par3 = m_gpf_pixelplt->getParameter<GSLibParGrid*>(3);
    par3->_specs_x->getParameter<GSLibParUInt*>(0)->_value = m_varmap_grid->getNX(); //nx
    par3->_specs_x->getParameter<GSLibParDouble*>(1)->_value = m_varmap_grid->getX0(); //min x
    par3->_specs_x->getParameter<GSLibParDouble*>(2)->_value = m_varmap_grid->getDX(); //cell size x
    par3->_specs_y->getParameter<GSLibParUInt*>(0)->_value = m_varmap_grid->getNY(); //ny
    par3->_specs_y->getParameter<GSLibParDouble*>(1)->_value = m_varmap_grid->getY0(); //min y
    par3->_specs_y->getParameter<GSLibParDouble*>(2)->_value = m_varmap_grid->getDY(); //cell size y
    par3->_specs_z->getParameter<GSLibParUInt*>(0)->_value = m_varmap_grid->getNZ(); //nz
    par3->_specs_z->getParameter<GSLibParDouble*>(1)->_value = m_varmap_grid->getZ0(); //min z
    par3->_specs_z->getParameter<GSLibParDouble*>(2)->_value = m_varmap_grid->getDZ(); //cell size z

    //----------------------------------------------------------------------------------

    //Generate the parameter file
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    m_gpf_pixelplt->save( par_file_path );

    //run pixelplt program
    Application::instance()->logInfo("Starting pixelplt program...");
    GSLib::instance()->runProgram( "pixelplt", par_file_path );

    GSLibParameterFile gpf = *m_gpf_pixelplt;

    //display the plot output
    DisplayPlotDialog *dpd = new DisplayPlotDialog(m_gpf_pixelplt->getParameter<GSLibParFile*>(1)->_path,
                                                   m_gpf_pixelplt->getParameter<GSLibParString*>(6)->_value,
                                                   gpf,
                                                   this);
    dpd->show();
}

void VariogramAnalysisDialog::onOpenExperimentalVariogramParamaters()
{
    File* parent_file = static_cast<File*>( m_head->getContainingFile() );
    if( parent_file->getFileType().compare("POINTSET") == 0 )
        onGamv();
    else
        onGam();
}

void VariogramAnalysisDialog::onOpenExperimentalVariogramPlot()
{
    File* parent_file = static_cast<File*>( m_head->getContainingFile() );
    if( parent_file->getFileType().compare("POINTSET") == 0 )
        onVargpltExperimentalIrregular();
    else
        onVargpltExperimentalRegular();
}

void VariogramAnalysisDialog::onSaveVarmapGrid()
{
    if( ! m_varmap_grid ){
        QMessageBox::critical( this, "Error", "You must first compute the variogram map at least once.");
        return;
    }

    bool ok;
    QString proposed_name( m_head->getName() );
    if( m_tail != m_head )
    {
        proposed_name.append( "_X_" );
        proposed_name.append( m_tail->getName() );
    }
    proposed_name = proposed_name.append("_VARMAP");
    QString new_plot_name = QInputDialog::getText(this, "Name the new grid file",
                                             "New grid file name:", QLineEdit::Normal,
                                             proposed_name, &ok);
    if (ok && !new_plot_name.isEmpty()){
        Application::instance()->getProject()->importCartesianGrid( m_varmap_grid, new_plot_name.append(".dat") );
    }

}

void VariogramAnalysisDialog::onSaveExpVariogram()
{
    if( !m_gpf_gam and !m_gpf_gamv ){
        QMessageBox::critical( this, "Error", "You must first compute the experimental variogram at least once.");
        return;
    }

    //the path to the generated output depends whether the user run gam or gamv
    QString _exp_var_file_path;
    if( m_gpf_gam )
        _exp_var_file_path = m_gpf_gam->getParameter<GSLibParFile*>(3)->_path;
    if( m_gpf_gamv )
        _exp_var_file_path = m_gpf_gamv->getParameter<GSLibParFile*>(4)->_path;

    //propose a name for the file
    QString proposed_name( m_head->getName() );
    if( m_tail != m_head )
    {
        proposed_name.append( "_X_" );
        proposed_name.append( m_tail->getName() );
    }
    proposed_name = proposed_name.append("_EXP_VARIOGRAM");

    //open file rename dialog
    bool ok;
    QString new_exp_vario_name = QInputDialog::getText(this, "Name the new experimental variogram file",
                                             "New experimental variogram file name:", QLineEdit::Normal,
                                             proposed_name, &ok);
    if (ok && !new_exp_vario_name.isEmpty()){

        //Generate a vargplt parameter file path (tmp directory)
        QDir path_for_varplt_file( Application::instance()->getProject()->getPath() );
        QString vargplt_par_file = path_for_varplt_file.filePath( new_exp_vario_name + ".vargplt" );

        //add an extension to the user-entered filename
        new_exp_vario_name.append(".out");

        //vargplt: replace the path to the experimental variogram data with the definitive one
        GSLibParRepeat *par6 = m_gpf_vargplt_exp_irreg->getParameter<GSLibParRepeat*>(6);
        uint ncurves = par6->getCount();
        for(uint i = 0; i < ncurves; ++i)
        {
            QDir path_to_exp_variogram_data( Application::instance()->getProject()->getPath() );
            par6->getParameter<GSLibParFile*>(i, 0)->_path = path_to_exp_variogram_data.filePath( new_exp_vario_name );
        }

        //vargplt: save the vargplt parameter file that will use the experimental variogram data saved to the project
        m_gpf_vargplt_exp_irreg->save( vargplt_par_file );

        //import the experimental variogram file to the project directory
        Application::instance()->getProject()->importExperimentalVariogram( _exp_var_file_path,
                                                                            new_exp_vario_name,
                                                                            vargplt_par_file );
    }
}

void VariogramAnalysisDialog::onOpenVariogramModelParamateres()
{
    if( !m_gpf_gam and !m_gpf_gamv and !m_ev ){
        QMessageBox::critical( this, "Error", "To perform variogam fitting, you must first compute the experimental variogram at least once.");
        return;
    }

    //suggest number of directions based on the previous experimental variogram computation
    uint ndir = 1; //default for variogram fitting mode
    if( m_gpf_gamv )
        ndir = m_gpf_gamv->getParameter<GSLibParUInt*>(8)->_value; //ndir
    if( m_gpf_gam )
    {
        GSLibParMultiValuedFixed *par6 = m_gpf_gam->getParameter<GSLibParMultiValuedFixed*>(6);
        ndir = par6->getParameter<GSLibParUInt*>(0)->_value; //ndir
    }

    //suggest number of lags based on the previous experimental variogram computation
    uint nlags = 10; //default for variogram fitting mode
    if( m_gpf_gamv )
        nlags = m_gpf_gamv->getParameter<GSLibParUInt*>(5)->_value;
    if( m_gpf_gam )
    {
        GSLibParMultiValuedFixed *par6 = m_gpf_gam->getParameter<GSLibParMultiValuedFixed*>(6);
        nlags = par6->getParameter<GSLibParUInt*>(1)->_value;
    }

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

    //suggest lag distances based on the previous experimental variogram computation
    QList<double> lags;
    if( m_gpf_gamv )
    {
        for(uint i = 0; i < ndir; ++i)
        {
            lags.append( m_gpf_gamv->getParameter<GSLibParDouble*>(6)->_value );
        }
    }
    if( m_gpf_gam )
    {
        //compute lag distance from the grid cell dimensions and steps
        GSLibParRepeat *par7 = m_gpf_gam->getParameter<GSLibParRepeat*>(7); //repeat ndir-times
        for(uint i = 0; i < ndir; ++i)
        {
            GSLibParMultiValuedFixed *par7_0 = par7->getParameter<GSLibParMultiValuedFixed*>(i, 0);
            int xstep = par7_0->getParameter<GSLibParInt*>(0)->_value;
            int ystep = par7_0->getParameter<GSLibParInt*>(1)->_value;
            int zstep = par7_0->getParameter<GSLibParInt*>(2)->_value;
            double xlag = xstep * xsize;
            double ylag = ystep * ysize;
            double zlag = zstep * zsize;
            double lag = sqrt( xlag*xlag + ylag*ylag + zlag*zlag );
            lags.append( lag );
        }
    }
    if( m_ev )
    {
        for(uint i = 0; i < ndir; ++i)
        {
            lags.append( 10.0 ); //default for variogram fitting mode
        }
    }

    //suggest azimuths based on the previous experimental variogram computation
    QList<double> azimuths;
    if( m_gpf_gamv )
    {
        GSLibParRepeat *par9 = m_gpf_gamv->getParameter<GSLibParRepeat*>(9); //repeat ndir-times
        for(uint i = 0; i < ndir; ++i)
        {
            GSLibParMultiValuedFixed *par9_0 = par9->getParameter<GSLibParMultiValuedFixed*>(i, 0);
            azimuths.append( par9_0->getParameter<GSLibParDouble*>(0)->_value );
        }
    }
    if( m_gpf_gam )
    {
        //compute azimuth from the grid cell dimensions and steps
        GSLibParRepeat *par7 = m_gpf_gam->getParameter<GSLibParRepeat*>(7); //repeat ndir-times
        for(uint i = 0; i < ndir; ++i)
        {
            GSLibParMultiValuedFixed *par7_0 = par7->getParameter<GSLibParMultiValuedFixed*>(i, 0);
            int xstep = par7_0->getParameter<GSLibParInt*>(0)->_value;
            int ystep = par7_0->getParameter<GSLibParInt*>(1)->_value;
            //refer to gam program instructions for cell steps equivalency to angles.
            double azimuth = 0.0; //azimuth defaults to zero
            if( xstep == 0 ) //azimuth along x axis
            {
                if( ystep < 0 ) azimuth = 180.0; else azimuth = 0.0;
            }
            else if( ystep == 0 ) //azimuth along y axis
            {
                if( xstep < 0 ) azimuth = 270.0; else azimuth = 90.0;
            }
            else if( xstep > 0 && ystep > 0 ) //azimuth in 1st quadrant
                azimuth = atan( xstep*xsize / ystep*ysize ) * C_180_OVER_PI;
            else if( xstep > 0 && ystep < 0 ) //azimuth in 2nd quadrant
                azimuth = 180.0 + atan( xstep*xsize / ystep*ysize ) * C_180_OVER_PI;
            else if( xstep < 0 && ystep < 0 ) //azimuth in 3rd quadrant
                azimuth = 180.0 + atan( xstep*xsize / ystep*ysize ) * C_180_OVER_PI;
            else if( xstep < 0 && ystep > 0 ) //azimuth in 4th quadrant
                azimuth = 360.0 + atan( xstep*xsize / ystep*ysize ) * C_180_OVER_PI;
            azimuths.append( azimuth );
        }
    }
    if( m_ev )
    {
        for(uint i = 0; i < ndir; ++i)
        {
            azimuths.append( 0.0 ); //default for variogram fitting mode
        }
    }

    //suggest dip angles based on the previous experimental variogram computation
    QList<double> dips;
    if( m_gpf_gamv )
    {
        GSLibParRepeat *par9 = m_gpf_gamv->getParameter<GSLibParRepeat*>(9); //repeat ndir-times
        for(uint i = 0; i < ndir; ++i)
        {
            GSLibParMultiValuedFixed *par9_0 = par9->getParameter<GSLibParMultiValuedFixed*>(i, 0);
            dips.append( par9_0->getParameter<GSLibParDouble*>(3)->_value );
        }
    }
    if( m_gpf_gam )
    {
        //compute dip from the grid cell dimensions and steps
        GSLibParRepeat *par7 = m_gpf_gam->getParameter<GSLibParRepeat*>(7); //repeat ndir-times
        for(uint i = 0; i < ndir; ++i)
        {
            GSLibParMultiValuedFixed *par7_0 = par7->getParameter<GSLibParMultiValuedFixed*>(i, 0);
            int xstep = par7_0->getParameter<GSLibParInt*>(0)->_value;
            int ystep = par7_0->getParameter<GSLibParInt*>(1)->_value;
            int zstep = par7_0->getParameter<GSLibParInt*>(2)->_value;
            double xlag = xstep * xsize;
            double ylag = ystep * ysize;
            double xylag = sqrt( xlag*xlag + ylag*ylag );
            double zlag = zstep * zsize;
            //refer to gam program instructions for cell steps equivalency to angles.
            double dip = 0.0; //dip defaults to zero
            if( xstep == 0 && ystep == 0) //dip along z axis
            {
                if( zstep < 0 ) dip = 90.0; else dip = -90.0;
            }
            else
                dip = -atan( zlag / xylag ) * C_180_OVER_PI;
            dips.append( dip );
        }
    }
    if( m_ev )
    {
        for(uint i = 0; i < ndir; ++i)
        {
            dips.append( 0.0 ); //default for variogram fitting mode
        }
    }

    //now that the geometric parameters were defined, we can make
    //a vmodel parameters object
    if( ! m_gpf_vmodel )
    {
        //Construct an object composition based on the parameter file template for the vmodel program.
        m_gpf_vmodel = new GSLibParameterFile( "vmodel" );
        //Set default values so we need to change less parameters and let
        //the user change the others as one may see fit.
        m_gpf_vmodel->setDefaultValues();

        m_gpf_vmodel->getParameter<GSLibParFile*>(0)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("var");

        GSLibParMultiValuedFixed *par1 = m_gpf_vmodel->getParameter<GSLibParMultiValuedFixed*>(1);
        par1->getParameter<GSLibParUInt*>(0)->_value = ndir; //ndir
        par1->getParameter<GSLibParUInt*>(1)->_value = nlags;

        //suggests azimuths, dips and lags based on the experimental variogram computation parameters
        GSLibParRepeat *par2 = m_gpf_vmodel->getParameter<GSLibParRepeat*>(2); //repeat ndir-times
        par2->setCount( ndir );
        for( uint i = 0; i < ndir; ++i)
        {
            GSLibParMultiValuedFixed *par2_0 = par2->getParameter<GSLibParMultiValuedFixed*>(i, 0);
            par2_0->getParameter<GSLibParDouble*>(0)->_value = azimuths[i];
            par2_0->getParameter<GSLibParDouble*>(1)->_value = dips[i];
            par2_0->getParameter<GSLibParDouble*>(2)->_value = lags[i];
        }
    }

    //show the parameter dialog so the user can review and adjust other settings before running vmodel
    GSLibParametersDialog gslibpardiag( m_gpf_vmodel );
    int result = gslibpardiag.exec();
    if( result == QDialog::Accepted ){
        //Generate the parameter file
        QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
        m_gpf_vmodel->save( par_file_path );
        //to be notified when vmap completes.
        connect( GSLib::instance(), SIGNAL(programFinished()), this, SLOT(onVmodelCompletion()) );
        //run vmodel program asynchronously (user can see the program outputs while it runs)
        Application::instance()->logInfo("Starting vmodel program...");
        GSLib::instance()->runProgramAsync( "vmodel", par_file_path );
    }
}

void VariogramAnalysisDialog::onVmodelCompletion()
{
    //frees all signal connections to the GSLib singleton.
    GSLib::instance()->disconnect();
    if( ! m_gpf_vmodel )
    {
        QMessageBox::critical( this, "Error", "You must first model the variogram at least once.");
        return;
    }
    if( m_gpf_gamv ){
        onVargplt( m_gpf_gamv->getParameter<GSLibParFile*>(4)->_path, m_gpf_vmodel->getParameter<GSLibParFile*>(0)->_path );
    }
    if( m_gpf_gam ){
        onVargplt( m_gpf_gam->getParameter<GSLibParFile*>(3)->_path, m_gpf_vmodel->getParameter<GSLibParFile*>(0)->_path );
    }
    if( m_ev ){
        onVargplt( m_ev->getPath(), m_gpf_vmodel->getParameter<GSLibParFile*>(0)->_path );
    }
}

void VariogramAnalysisDialog::onSaveVariogramModel()
{
    if( !m_gpf_vmodel ){
        QMessageBox::critical( this, "Error", "You must first model a variogram at least once.");
        return;
    }

    //Generate the parameter file in the tmp directory as if we were about to run vmodel
    QString var_model_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    m_gpf_vmodel->save( var_model_file_path );

    //propose a name for the file
    QString proposed_name;
    if( m_head )
        proposed_name.append( m_head->getName() );
    else
        proposed_name.append( m_ev->getName() );
    if( m_tail != m_head )
    {
        proposed_name.append( "_X_" );
        proposed_name.append( m_tail->getName() );
    }
    proposed_name = proposed_name.append("_VARIOGRAM_MODEL");

    //open file rename dialog
    bool ok;
    QString new_var_model_name = QInputDialog::getText(this, "Name the new variogram model file",
                                             "New variogram model file name:", QLineEdit::Normal,
                                             proposed_name, &ok);
    if (ok && !new_var_model_name.isEmpty()){
        Application::instance()->getProject()->importVariogramModel( var_model_file_path, new_var_model_name.append(".vmodel") );
    }
}

void VariogramAnalysisDialog::onGamv()
{
    //if the parameter file object was not constructed
    if( ! m_gpf_gamv ){

        //Construct an object composition based on the parameter file template for the gamv program.
        m_gpf_gamv = new GSLibParameterFile( "gamv" );

        //get input data file
        //the parent component of an attribute is a file
        //assumes the file is a Point Set, since the user is calling gamv
        PointSet* input_data_file = (PointSet*)m_head->getContainingFile();

        //loads data in file, because it's necessary.
        input_data_file->loadData();

        //get the variable index(es) in parent data file
        uint head_var_index = input_data_file->getFieldGEOEASIndex( m_head->getName() );
        uint tail_var_index = head_var_index; //default is auto variogram
        if( isCrossVariography() )
            tail_var_index = input_data_file->getFieldGEOEASIndex( m_tail->getName() );

        //Set default values so we need to change less parameters and let
        //the user change the others as one may see fit.
        m_gpf_gamv->setDefaultValues();

        //get the max and min of the selected variables
        double data_min = input_data_file->min( head_var_index-1 );
        double data_max = input_data_file->max( head_var_index-1 );
        if( isCrossVariography() ){
            double tail_min = input_data_file->min( tail_var_index-1 );
            double tail_max = input_data_file->max( tail_var_index-1 );
            if( tail_min < data_min )
                data_min = tail_min;
            if( tail_max > data_max )
                data_max = tail_max;
        }

        //----------------set the minimum required gamv paramaters-----------------------
        //input file
        m_gpf_gamv->getParameter<GSLibParFile*>(0)->_path = input_data_file->getPath();

        //X, Y and Z variables
        GSLibParMultiValuedFixed* par1 = m_gpf_gamv->getParameter<GSLibParMultiValuedFixed*>(1);
        par1->getParameter<GSLibParUInt*>(0)->_value = input_data_file->getXindex();
        par1->getParameter<GSLibParUInt*>(1)->_value = input_data_file->getYindex();
        par1->getParameter<GSLibParUInt*>(2)->_value = input_data_file->getZindex();

        //variable to compute variogram for
        GSLibParMultiValuedFixed *par2 = m_gpf_gamv->getParameter<GSLibParMultiValuedFixed*>(2);
        par2->getParameter<GSLibParUInt*>(0)->_value = 1; //number of variables
        if( isCrossVariography() )
            par2->getParameter<GSLibParUInt*>(0)->_value = 2; //number of variables
        GSLibParMultiValuedVariable *par2_1 = par2->getParameter<GSLibParMultiValuedVariable*>(1);
        par2_1->getParameter<GSLibParUInt*>(0)->_value = head_var_index;
        if( isCrossVariography() ){
            par2_1->assure( 2 );
            par2_1->getParameter<GSLibParUInt*>(1)->_value = tail_var_index;
        }

        //trimming limits
        GSLibParMultiValuedFixed *par3 = m_gpf_gamv->getParameter<GSLibParMultiValuedFixed*>(3);
        par3->getParameter<GSLibParDouble*>(0)->_value = data_min;
        par3->getParameter<GSLibParDouble*>(1)->_value = data_max;

        //define a non-default variogram option if the user opted for cross variography
        if( isCrossVariography() ){
            GSLibParRepeat *par12 = m_gpf_gamv->getParameter<GSLibParRepeat*>(12);
            GSLibParMultiValuedFixed *par12_0 = par12->getParameter<GSLibParMultiValuedFixed*>(0, 0);
            par12_0->getParameter<GSLibParUInt*>(0)->_value = 1;
            par12_0->getParameter<GSLibParUInt*>(1)->_value = 2;
            par12_0->getParameter<GSLibParOption*>(2)->_selected_value = 2;
            par12_0->getParameter<GSLibParDouble*>(3)->_value = 0.0;
        }

        //output file with experimental variogram values
        m_gpf_gamv->getParameter<GSLibParFile*>(4)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("out");
    }

    //--------------------------------------------------------------------------------

    //show the parameter dialog so the user can adjust other settings before running gamv
    GSLibParametersDialog gslibpardiag( m_gpf_gamv );
    int result = gslibpardiag.exec();
    if( result == QDialog::Accepted ){
        //Generate the parameter file
        QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
        m_gpf_gamv->save( par_file_path );
        //to be notified when gamv completes.
        connect( GSLib::instance(), SIGNAL(programFinished()), this, SLOT(onGamvCompletion()) );
        //run gamv program asynchronously (user can see the program outputs while it runs)
        Application::instance()->logInfo("Starting gamv program...");
        GSLib::instance()->runProgramAsync( "gamv", par_file_path );
    }
}

void VariogramAnalysisDialog::onGamvCompletion()
{
    //frees all signal connections to the GSLib singleton.
    GSLib::instance()->disconnect();
    //when gamv completes, call vargplt.
    onVargpltExperimentalIrregular();
}

void VariogramAnalysisDialog::onVarmapCompletion()
{
    //frees all signal connections to the GSLib singleton.
    GSLib::instance()->disconnect();
    //when varmap completes, call pixelplt to plot the varmap grid.
    onOpenVarMapPlot();
}

void VariogramAnalysisDialog::onVargpltExperimentalIrregular()
{
    if( ! m_gpf_gamv ){
        QMessageBox::critical( this, "Error", "You must first compute the experimental variogram at least once.");
        return;
    }
    onVargplt( m_gpf_gamv->getParameter<GSLibParFile*>(4)->_path, "" );
}

void VariogramAnalysisDialog::onVargpltExperimentalRegular()
{
    if( ! m_gpf_gam ){
        QMessageBox::critical( this, "Error", "You must first compute the experimental variogram at least once.");
        return;
    }
    onVargplt( m_gpf_gam->getParameter<GSLibParFile*>(3)->_path, "" );
}

void VariogramAnalysisDialog::onVargplt(const QString path_to_exp_variogram_data , const QString path_to_vmodel_output)
{

    //compute a number of variogram curves to plot depending on
    //the experimental variogram calculation parameters
    uint ncurves = 1; //default is one variogram to plot
    if( m_gpf_gamv )
        ncurves = m_gpf_gamv->getParameter<GSLibParUInt*>(8)->_value * m_gpf_gamv->getParameter<GSLibParUInt*>(11)->_value;
    if( m_gpf_gam )
    {
        GSLibParMultiValuedFixed *par6 = m_gpf_gam->getParameter<GSLibParMultiValuedFixed*>(6);
        ncurves = par6->getParameter<GSLibParUInt*>(0)->_value * m_gpf_gam->getParameter<GSLibParUInt*>(9)->_value;
    }
    if( m_ev )
    {
        //the number of curves of a saved experimental variogram likely matches
        //the number given in its accessory vargplt parameter file in the project
        GSLibParameterFile gpf_accessory_vargplt( "vargplt" );
        //loads the data from the accessory vargplt parameter file
        gpf_accessory_vargplt.setValuesFromParFile( m_ev->getPathToVargpltPar() );
        //get the number of curves from it
        ncurves = gpf_accessory_vargplt.getParameter<GSLibParUInt*>(1)->_value;
    }
    //if a variogram model is to be displayed during variogram model fitting, then
    //the number of curved increases accordingly
    uint ncurves_models = 0; //default is no variogram model curve to plot
    if( ! path_to_vmodel_output.isEmpty() )
    {
        GSLibParMultiValuedFixed *par1 = m_gpf_vmodel->getParameter<GSLibParMultiValuedFixed*>(1);
        ncurves_models = par1->getParameter<GSLibParUInt*>(0)->_value;
    }

    //make a GLSib parameter object if it wasn't done yet.
    if( ! m_gpf_vargplt_exp_irreg ){
        m_gpf_vargplt_exp_irreg = new GSLibParameterFile( "vargplt" );

        //Set default values so we need to change less parameters and let
        //the user change the others as one may see fit.
        m_gpf_vargplt_exp_irreg->setDefaultValues();

        //make plot/window title
        QString title;
        if( m_head && m_tail )
        {
            title.append( m_head->getContainingFile()->getName() );
            title.append(" variogram: ");
            title.append(m_head->getName());
            title.append("(u) x ");
            title.append(m_tail->getName());
            title.append("(u+h)");
        }
        if( m_ev )
        {
            title.append( "Variogram model for " );
            title.append( m_ev->getName() );
        }

        //--------------------set some parameter values-----------------------

        //postscript file
        m_gpf_vargplt_exp_irreg->getParameter<GSLibParFile*>(0)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("ps");

        //suggest display settings for each variogram curve
        GSLibParRepeat *par6 = m_gpf_vargplt_exp_irreg->getParameter<GSLibParRepeat*>(6); //repeat nvarios-times
        par6->setCount( ncurves + ncurves_models );
        //the experimental curves
        for(uint i = 0; i < ncurves; ++i)
        {
            par6->getParameter<GSLibParFile*>(i, 0)->_path = path_to_exp_variogram_data;
            GSLibParMultiValuedFixed *par6_0_1 = par6->getParameter<GSLibParMultiValuedFixed*>(i, 1);
            par6_0_1->getParameter<GSLibParUInt*>(0)->_value = i + 1;
            par6_0_1->getParameter<GSLibParUInt*>(1)->_value = 1;
            par6_0_1->getParameter<GSLibParOption*>(2)->_selected_value = 1;
            par6_0_1->getParameter<GSLibParOption*>(3)->_selected_value = 0;
            par6_0_1->getParameter<GSLibParColor*>(4)->_color_code = (i % 15) + 1; //cycle through the available colors (except the gray tones)
        }

        //plot title
        m_gpf_vargplt_exp_irreg->getParameter<GSLibParString*>(5)->_value = title;
    }

    //suggest (number of directions * number of variograms) from the variogram calculation parameters
    m_gpf_vargplt_exp_irreg->getParameter<GSLibParUInt*>(1)->_value = ncurves + ncurves_models; // nvarios

    //adjust curve count as needed
    GSLibParRepeat *par6 = m_gpf_vargplt_exp_irreg->getParameter<GSLibParRepeat*>(6); //repeat nvarios-times
    uint old_count = par6->getCount();
    par6->setCount( ncurves + ncurves_models );

    //determine wether the number of curves changed
    bool curve_count_changed = ( old_count != ncurves + ncurves_models );

    //suggest model curves visual parameters after vmodel runs (only if curve count changed)
    for(uint i = ncurves; i < (ncurves + ncurves_models) && curve_count_changed; ++i)
    {
        par6->getParameter<GSLibParFile*>(i, 0)->_path = path_to_vmodel_output;
        GSLibParMultiValuedFixed *par6_0_1 = par6->getParameter<GSLibParMultiValuedFixed*>(i, 1);
        par6_0_1->getParameter<GSLibParUInt*>(0)->_value = i - ncurves + 1;
        par6_0_1->getParameter<GSLibParUInt*>(1)->_value = 0;
        par6_0_1->getParameter<GSLibParOption*>(2)->_selected_value = 0;
        par6_0_1->getParameter<GSLibParOption*>(3)->_selected_value = 1;
        par6_0_1->getParameter<GSLibParColor*>(4)->_color_code = ((i - ncurves) % 15) + 1; //cycle through the available colors (except the gray tones)
    }

    //input is the output of gamv/gam
    //FIXME: handle different experimental variogram data files (repeat tag)
    for(uint i = 0; i < ncurves; ++i)
    {
        par6->getParameter<GSLibParFile*>(i, 0)->_path = path_to_exp_variogram_data;
    }
    for(uint i = ncurves; i < (ncurves + ncurves_models); ++i)
    {
        par6->getParameter<GSLibParFile*>(i, 0)->_path = path_to_vmodel_output;
    }

    //----------------------display plot------------------------------------------------------------

    //Generate the parameter file
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    m_gpf_vargplt_exp_irreg->save( par_file_path );

    //run vargplt program
    Application::instance()->logInfo("Starting vargplt program...");
    GSLib::instance()->runProgram( "vargplt", par_file_path );

    GSLibParameterFile gpf = *m_gpf_vargplt_exp_irreg;

    //display the plot output
    DisplayPlotDialog *dpd = new DisplayPlotDialog(m_gpf_vargplt_exp_irreg->getParameter<GSLibParFile*>(0)->_path,
                                                   m_gpf_vargplt_exp_irreg->getParameter<GSLibParString*>(5)->_value,
                                                   gpf,
                                                   this);
    dpd->show();
}

void VariogramAnalysisDialog::onGam()
{
    //if the parameter file object was not constructed
    if( ! m_gpf_gam ){

        //Construct an object composition based on the parameter file template for the gamv program.
        m_gpf_gam = new GSLibParameterFile( "gam" );

        //get input data file
        //the parent component of an attribute is a file
        //assumes the file is a Point Set, since the user is calling gamv
        CartesianGrid* input_data_file = (CartesianGrid*)m_head->getContainingFile();

        //loads data in file.
        input_data_file->loadData();

        //get the variable index(es) in parent data file
        uint head_var_index = input_data_file->getFieldGEOEASIndex( m_head->getName() );
        uint tail_var_index = head_var_index; //default is auto variogram
        if( isCrossVariography() )
            tail_var_index = input_data_file->getFieldGEOEASIndex( m_tail->getName() );

        //Set default values so we need to change less parameters and let
        //the user change the others as one may see fit.
        m_gpf_gam->setDefaultValues();

        //get the max and min of the selected variable(s)
        double data_min = input_data_file->min( head_var_index-1 );
        double data_max = input_data_file->max( head_var_index-1 );
        if( isCrossVariography() ){
            double tail_min = input_data_file->min( tail_var_index-1 );
            double tail_max = input_data_file->max( tail_var_index-1 );
            if( tail_min < data_min )
                data_min = tail_min;
            if( tail_max > data_max )
                data_max = tail_max;
        }

        //----------------set the minimum required gam paramaters-----------------------
        //input file
        m_gpf_gam->getParameter<GSLibParFile*>(0)->_path = input_data_file->getPath();

        //variable to compute variogram for
        GSLibParMultiValuedFixed *par1 = m_gpf_gam->getParameter<GSLibParMultiValuedFixed*>(1);
        par1->getParameter<GSLibParUInt*>(0)->_value = 1; //number of variables
        if( isCrossVariography() )
            par1->getParameter<GSLibParUInt*>(0)->_value = 2; //number of variables
        GSLibParMultiValuedVariable *par1_1 = par1->getParameter<GSLibParMultiValuedVariable*>(1);
        par1_1->getParameter<GSLibParUInt*>(0)->_value = head_var_index;
        if( isCrossVariography() ){
            par1_1->assure( 2 );
            par1_1->getParameter<GSLibParUInt*>(1)->_value = tail_var_index;
        }

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

        //set non-default variogam options if the user opted for cross variography
        if( isCrossVariography() ){
            GSLibParRepeat *par10 = m_gpf_gam->getParameter<GSLibParRepeat*>(10); //repeat nvarios-times
            GSLibParMultiValuedFixed *par10_0 = par10->getParameter<GSLibParMultiValuedFixed*>(0, 0);
            par10_0->getParameter<GSLibParUInt*>(0)->_value = 1;
            par10_0->getParameter<GSLibParUInt*>(1)->_value = 2;
            par10_0->getParameter<GSLibParOption*>(2)->_selected_value = 2;
            par10_0->getParameter<GSLibParDouble*>(3)->_value = 0.0;
        }
    }
    //--------------------------------------------------------------------------------

    //show the parameter dialog so the user can adjust other settings before running gam
    GSLibParametersDialog gslibpardiag( m_gpf_gam );
    int result = gslibpardiag.exec();
    if( result == QDialog::Accepted ){
        //Generate the parameter file
        QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
        m_gpf_gam->save( par_file_path );
        //run gam program
        Application::instance()->logInfo("Starting gam program...");
        GSLib::instance()->runProgram( "gam", par_file_path );
        onVargpltExperimentalRegular();
    }
}
