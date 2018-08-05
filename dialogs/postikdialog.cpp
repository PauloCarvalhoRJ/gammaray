#include "postikdialog.h"

#include <QMessageBox>
#include <QInputDialog>

#include "ui_postikdialog.h"
#include "domain/application.h"
#include "domain/cartesiangrid.h"
#include "domain/thresholdcdf.h"
#include "domain/project.h"
#include "widgets/cartesiangridselector.h"
#include "widgets/fileselectorwidget.h"
#include "widgets/variableselector.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslibparametersdialog.h"
#include "gslib/gslib.h"
#include "util.h"

PostikDialog::PostikDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PostikDialog),
    m_gpf_postik( nullptr ),
    m_cg_postprocess( nullptr )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle("Indicator Kriging post-processing");

    //The list with existing grids in the project.
    m_Ik3dGridSelector = new CartesianGridSelector();
    ui->frmIk3dOutput->layout()->addWidget( m_Ik3dGridSelector );

    //the list with the threshold c.d.f.'s containing the thresholds
    m_thresholdsSelector = new FileSelectorWidget( FileSelectorType::CDFs );
    ui->frmThresholds->layout()->addWidget( m_thresholdsSelector );

    //the list with data files to inform the distribution (optional)
    m_fileForDistSelector = new FileSelectorWidget( FileSelectorType::DataFiles, true );
    ui->frmDataForDistribution->layout()->addWidget( m_fileForDistSelector );

    //The list with the variables to inform the distribution
    m_variableForDistSelector = new VariableSelector();
    ui->frmDataForDistribution->layout()->addWidget( m_variableForDistSelector );
    connect( m_fileForDistSelector, SIGNAL(dataFileSelected(DataFile*)),
             m_variableForDistSelector, SLOT(onListVariables(DataFile*)) );

    //The list with the declustering weight to inform the distribution
    m_weightForDistSelector = new VariableSelector( true );
    ui->frmDataForDistribution->layout()->addWidget( m_weightForDistSelector );
    connect( m_fileForDistSelector, SIGNAL(dataFileSelected(DataFile*)),
             m_weightForDistSelector, SLOT(onListVariables(DataFile*)) );

    //set high-res icons for high-dpi displays
    if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI ){
        ui->btnConfigAndRun->setIcon( QIcon(":icons32/setting32") );
        ui->btnSave->setIcon( QIcon(":icons32/save32") );
    }
    adjustSize();
}

PostikDialog::~PostikDialog()
{
    delete ui;
    Application::instance()->logInfo("PostikDialog destroyed.");
}

void PostikDialog::onConfigureAndRun()
{

    //get the selected ik3d grid file
    CartesianGrid *cg_ik3d = (CartesianGrid*)m_Ik3dGridSelector->getSelectedDataFile();
    if( ! cg_ik3d ){
        QMessageBox::critical( this, "Error", "Please, select a ik3d-generated Cartesian grid file.");
        return;
    }

    //get the selected c.d.f. file
    ThresholdCDF *distributionWithThresholds = (ThresholdCDF*)m_thresholdsSelector->getSelectedFile();
    if( ! distributionWithThresholds ){
        QMessageBox::critical( this, "Error", "Please, select a c.d.f file with the thresholds.");
        return;
    }

    if( ! m_gpf_postik ){
        m_gpf_postik = new GSLibParameterFile( "postik" );
        m_gpf_postik->setDefaultValues();

        //the postik output file
        m_gpf_postik->getParameter<GSLibParFile*>(1)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");

        //loads the threshold/cumulative frequencies pairs from file system.
        distributionWithThresholds->loadPairs();

        //get the first (lowest) threshold to suggest a lowest value for the interpolated cumulative distributions
        double lowest = distributionWithThresholds->get1stValue( 0 );
        lowest = lowest - (lowest / 10.0);

        //get the last (highest) threshold to suggest a highest value for the interpolated cumulative distributions
        double highest = distributionWithThresholds->get1stValue( distributionWithThresholds->getPairCount()-1 );
        highest = highest + (highest / 10.0);

        //the max/min Z values in the interpolated cumulative distributions
        GSLibParMultiValuedFixed *par8 = m_gpf_postik->getParameter<GSLibParMultiValuedFixed*>(8);
        par8->getParameter<GSLibParDouble*>(0)->_value = lowest;
        par8->getParameter<GSLibParDouble*>(1)->_value = highest;
    }


    //set the ik3d-generated Cartesian grid file
    m_gpf_postik->getParameter<GSLibParFile*>(0)->_path = cg_ik3d->getPath();

    //the number of thresholds
    m_gpf_postik->getParameter<GSLibParUInt*>(3)->_value = distributionWithThresholds->getPairCount();

    //the thresholds (get from the selected threshold c.d.f.)
    GSLibParMultiValuedVariable *par4 = m_gpf_postik->getParameter<GSLibParMultiValuedVariable*>(4);
    par4->setSize( m_gpf_postik->getParameter<GSLibParUInt*>(3)->_value );
    for( uint i = 0; i < m_gpf_postik->getParameter<GSLibParUInt*>(3)->_value; ++i){
        par4->getParameter<GSLibParDouble*>(i)->_value = distributionWithThresholds->get1stValue( i );
    }

    //if the user set a data file to better characterize the cumulative distribution (tabulated quantiles)...
    DataFile *cg_dataForDist = dynamic_cast<DataFile*>(m_fileForDistSelector->getSelectedFile());
    if( cg_dataForDist ){
        //the optional data to better inform the cumulative distribution
        m_gpf_postik->getParameter<GSLibParFile*>(6)->_path = cg_dataForDist->getPath();

        //get the selected variable and weight for tabulated quantiles
        uint dataForDist_var = m_variableForDistSelector->getSelectedVariableGEOEASIndex();
        uint dataForDist_wgt = m_weightForDistSelector->getSelectedVariableGEOEASIndex();

        //get the max/min of the data for tabulated quantiles
        cg_dataForDist->loadData();
        double dataForDist_min = cg_dataForDist->min( dataForDist_var-1 );
        double dataForDist_max = cg_dataForDist->max( dataForDist_var-1 );
        dataForDist_min = dataForDist_min - (dataForDist_min/100.0);
        dataForDist_max = dataForDist_max + (dataForDist_max/100.0);

        //the variable, the declustering weight and the trimming limits
        GSLibParMultiValuedFixed *par7 = m_gpf_postik->getParameter<GSLibParMultiValuedFixed*>(7);
        par7->getParameter<GSLibParUInt*>(0)->_value = dataForDist_var;
        par7->getParameter<GSLibParUInt*>(1)->_value = dataForDist_wgt;
        par7->getParameter<GSLibParDouble*>(2)->_value = dataForDist_min;
        par7->getParameter<GSLibParDouble*>(3)->_value = dataForDist_max;
    } else {
        m_gpf_postik->getParameter<GSLibParFile*>(6)->_path = "data_for_global_dist.dat";
    }

    //----------------------------prepare and execute postik--------------------------------

    //show the postik parameters
    GSLibParametersDialog gsd( m_gpf_postik, this );
    int result = gsd.exec();

    //if user didn't cancel the dialog
    if( result == QDialog::Accepted ){
        //Generate the parameter file
        QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "par" );
        m_gpf_postik->save( par_file_path );

        //to be notified when postik completes.
        connect( GSLib::instance(), SIGNAL(programFinished()), this, SLOT(onPostikCompletes()) );

        //run postik program asynchronously
        Application::instance()->logInfo("Starting postik program...");
        GSLib::instance()->runProgramAsync( "postik", par_file_path );
    }

}

void PostikDialog::onPostikCompletes()
{
    //frees all signal connections to the GSLib singleton.
    GSLib::instance()->disconnect();

    preview();
}

void PostikDialog::onSave()
{
    if( ! m_gpf_postik || ! m_cg_postprocess ){
        QMessageBox::critical( this, "Error", "Please, run the post-processing at least once.");
        return;
    }

    //get the selected estimation grid
    CartesianGrid* estimation_grid = (CartesianGrid*)m_Ik3dGridSelector->getSelectedDataFile();
    if( ! estimation_grid ){
        QMessageBox::critical( this, "Error", "Please, select an ik3d-generated Cartesian grid to set the grid geometry from.");
        return;
    }

    //suggest a name for the new grid based on which post-processed product the user selected
    QString new_cg_name = estimation_grid->getName();
    GSLibParMultiValuedFixed *par2 = m_gpf_postik->getParameter<GSLibParMultiValuedFixed*>(2);
    double parameter = par2->getParameter<GSLibParDouble*>(1)->_value;
    switch( par2->getParameter<GSLibParOption*>(0)->_selected_value ){
    case 1: //mean
        new_cg_name += "_eType";
        break;
    case 2: //prob. above and mean above/below threshold
        new_cg_name += "_ProbAndMeanAbove_" + QString::number( parameter );
        break;
    case 3: //quantile
        new_cg_name += "_P" + QString::number( parameter * 100 );
        break;
    case 4: //variance
        new_cg_name += "_Variance";
        break;
    }

    //presents a naming dialog with a suggested name for the new Cartesian grid.
    bool ok;
    new_cg_name = QInputDialog::getText(this, "Name the new cartesian grid",
                                             "Name for the new Cartesian grid:", QLineEdit::Normal,
                                             new_cg_name, &ok);

    //if the user didn't cancel the input dialog
    if( ok ){
        //get the postik output file
        QString tmp_file_path = m_gpf_postik->getParameter<GSLibParFile*>(1)->_path;

        //create a new grid object corresponding to the file with the post-processed product
        CartesianGrid* cg = new CartesianGrid( tmp_file_path );

        //set the metadata info from the post-processed grid
        cg->setInfoFromOtherCG( m_cg_postprocess, false );

        //import the grid file with the IK estimates as a project item
        Application::instance()->getProject()->importCartesianGrid( cg, new_cg_name );
    }

}

void PostikDialog::preview()
{
    if( m_cg_postprocess )
        delete m_cg_postprocess;

    //get the tmp file path created by postik
    QString grid_file_path = m_gpf_postik->getParameter<GSLibParFile*>(1)->_path;

    //create a new grid object corresponding to the file created by kt3d
    m_cg_postprocess = new CartesianGrid( grid_file_path );

    //set the grid geometry info.
    m_cg_postprocess->setInfoFromOtherCG( (CartesianGrid*)m_Ik3dGridSelector->getSelectedDataFile() );

    m_cg_postprocess->loadData();

    //get the number of attributes in the postik output
    uint nattr = m_cg_postprocess->getDataColumnCount();

    //for each attribute
    for( uint i = 0; i < nattr; ++i){
        //get the attribute
        Attribute* est_var = (Attribute*)m_cg_postprocess->getChildByIndex( i );
        //open the plot dialog
        Util::viewGrid( est_var, this );
    }

}
