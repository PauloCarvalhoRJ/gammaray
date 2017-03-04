#include "indicatorkrigingdialog.h"
#include "ui_indicatorkrigingdialog.h"
#include "widgets/cartesiangridselector.h"
#include "widgets/pointsetselector.h"
#include "widgets/variableselector.h"
#include "widgets/fileselectorwidget.h"
#include "widgets/variogrammodelselector.h"
#include "domain/file.h"
#include "domain/application.h"
#include "domain/project.h"
#include "domain/categorypdf.h"
#include "domain/thresholdcdf.h"
#include "domain/pointset.h"
#include "domain/variogrammodel.h"
#include "domain/cartesiangrid.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "gslib/gslibparametersdialog.h"
#include "gslib/gslib.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "util.h"

IndicatorKrigingDialog::IndicatorKrigingDialog(IKVariableType varType, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IndicatorKrigingDialog),
    m_gpf_ik3d( nullptr ),
    m_varType( varType ),
    m_cg_estimation( nullptr )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    //configure UI captions according to IK variable type
    if( varType == IKVariableType::CONTINUOUS ){
        this->setWindowTitle("Indicator kriging for a continuous variable.");
        ui->lblIKVarType->setText("<html><head/><body><p><span style=\" font-weight:600; color:#0000ff;\">CONTINUOUS</span></p></body></html>");
        ui->lblDistributionFile->setText("Threshold c.d.f. file:");
    } else {
        this->setWindowTitle("Indicator kriging for a categorical variable.");
        ui->lblIKVarType->setText("<html><head/><body><p><span style=\" font-weight:600; color:#0000ff;\">CATEGORICAL</span></p></body></html>");
        ui->lblDistributionFile->setText("Category p.d.f. file:");
    }

    //The list with existing point sets in the project.
    m_psSelector = new PointSetSelector();
    ui->frmInput->layout()->addWidget( m_psSelector );

    //The list with the Point Set variables to set the variable
    m_PointSetVariableSelector = new VariableSelector();
    ui->frmInput->layout()->addWidget( m_PointSetVariableSelector );
    connect( m_psSelector, SIGNAL(pointSetSelected(DataFile*)),
             m_PointSetVariableSelector, SLOT(onListVariables(DataFile*)) );

    //The list with existing point sets in the project (for the soft indicators)
    m_psSoftSelector = new PointSetSelector( true );
    ui->frmSoftIndicators->layout()->addWidget( m_psSoftSelector );
    connect( m_psSoftSelector, SIGNAL(pointSetSelected(DataFile*)),
             this, SLOT(onUpdateSoftIndicatorVariablesSelectors()) );

    //The list with existing c.d.f./p.d.f. files in the project.
    if( varType == IKVariableType::CONTINUOUS )
        m_dfSelector = new FileSelectorWidget( FileSelectorType::CDFs );
    else
        m_dfSelector = new FileSelectorWidget( FileSelectorType::PDFs );
    ui->frmDistribution->layout()->addWidget( m_dfSelector );
    connect( m_dfSelector, SIGNAL(fileSelected(File*)),
             this, SLOT(onUpdateVariogramSelectors()) );
    connect( m_dfSelector, SIGNAL(fileSelected(File*)),
             this, SLOT(onUpdateSoftIndicatorVariablesSelectors()) );

    //The list with existing cartesian grids in the project for the estimation.
    m_cgSelector = new CartesianGridSelector();
    ui->frmGrid->layout()->addWidget( m_cgSelector );

    //calling this slot causes the variable combobox to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_psSelector->onSelection( 0 );

    //call this slot to show the variogram selector widgets.
    onUpdateVariogramSelectors();

    //call this slot to show the soft indicator variables selectors.
    onUpdateSoftIndicatorVariablesSelectors();
    
    if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI ){
        ui->btnConfigureAndRun->setIcon( QIcon(":icons32/setting32") );
    }

    adjustSize();
}

IndicatorKrigingDialog::~IndicatorKrigingDialog()
{
    delete ui;
}

void IndicatorKrigingDialog::addVariogramSelector(){
    VariogramModelSelector* vms = new VariogramModelSelector();
    ui->groupVariograms->layout()->addWidget( vms );
    m_variogramSelectors.append( vms );
}

void IndicatorKrigingDialog::preview()
{
    if( m_cg_estimation )
        delete m_cg_estimation;

    //get the tmp file path created by ik3d with the p.d.f. estimates
    QString grid_file_path = m_gpf_ik3d->getParameter<GSLibParFile*>(14)->_path;

    //create a new grid object corresponding to the file created by kt3d
    m_cg_estimation = new CartesianGrid( grid_file_path );

    //set the grid geometry info.
    m_cg_estimation->setInfoFromGridParameter( m_gpf_ik3d->getParameter<GSLibParGrid*>(15) );

    //kt3d usually uses -999 as no-data-value.
    m_cg_estimation->setNoDataValue( "-999" );

    //get the number of classes/thresholds in the distribution file
    uint ndist = m_dfSelector->getSelectedFile()->getContentsCount();

    //for each class/threshold
    for( uint i = 0; i < ndist; ++i){
        //get the class/threshold probability field
        Attribute* est_var = (Attribute*)m_cg_estimation->getChildByIndex( i );
        //open the plot dialog
        Util::viewGrid( est_var, this );
    }
}

void IndicatorKrigingDialog::onUpdateVariogramSelectors()
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
        File* file = m_dfSelector->getSelectedFile();
        if( file ){
            file->readFromFS();
            int tot = file->getContentsCount();
            for( int i = 0; i < tot; ++i){
                addVariogramSelector();
            }
        }
    }
}

void IndicatorKrigingDialog::onConfigureAndRun()
{
    //-----------------------------set ik3d parameters---------------------------
    if( ! m_gpf_ik3d ){
        //create the parameters object
        m_gpf_ik3d = new GSLibParameterFile("ik3d");

        //set the default values, so we need to set fewer parameters here
        m_gpf_ik3d->setDefaultValues();

        //output file
        m_gpf_ik3d->getParameter<GSLibParFile*>(14)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");
    }

    //-----------------these parameters must be re-set according to what the user has selected in the dialog--------

    //get the selected p.d.f./c.d.f. file
    File *distribution = m_dfSelector->getSelectedFile();

    //ValuePairs class implements getContentsCount() to return the number of pairs.
    uint ndist = distribution->getContentsCount();

    //variable type
    if( m_varType == IKVariableType::CATEGORICAL )
        m_gpf_ik3d->getParameter<GSLibParOption*>(0)->_selected_value = 0;
    else
        m_gpf_ik3d->getParameter<GSLibParOption*>(0)->_selected_value = 1;

    //ndist
    m_gpf_ik3d->getParameter<GSLibParUInt*>(4)->_value = ndist; //ndist (number of thresholds/categories)

    //thresolds/classes and c.d.f./p.d.f. values
    GSLibParMultiValuedVariable *par5 = m_gpf_ik3d->getParameter<GSLibParMultiValuedVariable*>(5);
    GSLibParMultiValuedVariable *par6 = m_gpf_ik3d->getParameter<GSLibParMultiValuedVariable*>(6);
    par5->setSize( ndist );
    par6->setSize( ndist );
    for( uint i = 0; i < ndist; ++i){
        if( m_varType == IKVariableType::CATEGORICAL ){
            CategoryPDF *pdf = (CategoryPDF*)distribution;
            par5->getParameter<GSLibParDouble*>(i)->_value = pdf->get1stValue(i);
            par6->getParameter<GSLibParDouble*>(i)->_value = pdf->get2ndValue(i);
        } else {
            ThresholdCDF *cdf = (ThresholdCDF*)distribution;
            par5->getParameter<GSLibParDouble*>(i)->_value = cdf->get1stValue(i);
            par6->getParameter<GSLibParDouble*>(i)->_value = cdf->get2ndValue(i);
        }
    }

    PointSet *pointSet = (PointSet*)m_psSelector->getSelectedDataFile();

    int varIndex = m_PointSetVariableSelector->getSelectedVariableGEOEASIndex();

    //point set data file
    m_gpf_ik3d->getParameter<GSLibParFile*>(7)->_path = pointSet->getPath();

    //data file fields
    GSLibParMultiValuedFixed *par8 = m_gpf_ik3d->getParameter<GSLibParMultiValuedFixed*>(8);
    par8->getParameter<GSLibParUInt*>(0)->_value = 0; //DH
    par8->getParameter<GSLibParUInt*>(1)->_value = pointSet->getXindex(); //X
    par8->getParameter<GSLibParUInt*>(2)->_value = pointSet->getYindex(); //Y
    par8->getParameter<GSLibParUInt*>(3)->_value = pointSet->getZindex(); //Z
    par8->getParameter<GSLibParUInt*>(4)->_value = varIndex; //variable

    //load the data file and get min./max.
    pointSet->loadData();
    double min = pointSet->min( varIndex - 1 );
    double max = pointSet->max( varIndex - 1 );

    //the soft indicator file is surely a PointSet object
    PointSet *psSoftData = (PointSet*)m_psSoftSelector->getSelectedDataFile();
    if( psSoftData ){
        //the soft data file
        m_gpf_ik3d->getParameter<GSLibParFile*>(9)->_path = psSoftData->getPath();
        GSLibParMultiValuedFixed *par10 = m_gpf_ik3d->getParameter<GSLibParMultiValuedFixed*>(10);
        //x,y,z columns of the soft data file
        par10->getParameter<GSLibParUInt*>(0)->_value = psSoftData->getXindex();
        par10->getParameter<GSLibParUInt*>(1)->_value = psSoftData->getYindex();
        par10->getParameter<GSLibParUInt*>(2)->_value = psSoftData->getZindex();
        GSLibParMultiValuedVariable *par10_3 = par10->getParameter<GSLibParMultiValuedVariable*>(3);
        //the soft data fields
        par10_3->setSize( ndist );
        for( uint i = 0; i < ndist; ++i){
            par10_3->getParameter<GSLibParUInt*>(i)->_value =
                    m_SoftIndicatorVariablesSelectors[i]->getSelectedVariableGEOEASIndex();
        }
    } else { //if soft data is not set, make sure all fields are set to zero
        GSLibParMultiValuedFixed *par10 = m_gpf_ik3d->getParameter<GSLibParMultiValuedFixed*>(10);
        par10->getParameter<GSLibParUInt*>(0)->_value = 0;
        par10->getParameter<GSLibParUInt*>(1)->_value = 0;
        par10->getParameter<GSLibParUInt*>(2)->_value = 0;
    }

    //trimming limits
    GSLibParMultiValuedFixed *par11 = m_gpf_ik3d->getParameter<GSLibParMultiValuedFixed*>(11);
    par11->getParameter<GSLibParDouble*>(0)->_value = min;
    par11->getParameter<GSLibParDouble*>(1)->_value = max;

    //set the grid parameters
    m_gpf_ik3d->setGridParameters( (CartesianGrid*)m_cgSelector->getSelectedDataFile() );

    //IK mode (has to specify threshold if mode is median IK)
    GSLibParMultiValuedFixed *par20 = m_gpf_ik3d->getParameter<GSLibParMultiValuedFixed*>(20);
    if( ui->radioFullIK->isChecked() )
        par20->getParameter<GSLibParOption*>(0)->_selected_value = 0;
    else
        par20->getParameter<GSLibParOption*>(0)->_selected_value = 1;

    //set the variogram model(s)
    GSLibParRepeat *par22 = m_gpf_ik3d->getParameter<GSLibParRepeat*>(22);
    if( ui->radioMedianIK->isChecked() ){ //median IK requires just one variogram model
        par22->setCount( 1 );
        GSLibParVModel *par22_0 = par22->getParameter<GSLibParVModel*>(0, 0);
        VariogramModelSelector* vms = m_variogramSelectors.at( 0 );
        VariogramModel *vmodel = vms->getSelectedVModel();
        par22_0->setFromVariogramModel( vmodel );
    } else { //full IK requires one variogram model per c.d.f./p.d.f. threshold/class.
        par22->setCount( ndist );
        for( uint i = 0; i < ndist; ++i){
            GSLibParVModel *par22_0 = par22->getParameter<GSLibParVModel*>(i, 0);
            VariogramModelSelector* vms = m_variogramSelectors.at( i );
            VariogramModel *vmodel = vms->getSelectedVModel();
            par22_0->setFromVariogramModel( vmodel );
        }
    }

    //----------------------------prepare and execute ik3d--------------------------------

    //show the ik3d parameters
    GSLibParametersDialog gsd( m_gpf_ik3d, this );
    int result = gsd.exec();

    //if user didn't cancel the dialog
    if( result == QDialog::Accepted ){
        //Generate the parameter file
        QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "par" );
        m_gpf_ik3d->save( par_file_path );

        //to be notified when ik3d completes.
        connect( GSLib::instance(), SIGNAL(programFinished()), this, SLOT(onIk3dCompletes()) );

        //run ik3d program asynchronously
        Application::instance()->logInfo("Starting ik3d program...");
        GSLib::instance()->runProgramAsync( "ik3d", par_file_path );
    }
}

void IndicatorKrigingDialog::onIk3dCompletes()
{
    //frees all signal connections to the GSLib singleton.
    GSLib::instance()->disconnect();

    preview();
}

void IndicatorKrigingDialog::onUpdateSoftIndicatorVariablesSelectors()
{
    //clears the current soft indicator variable selectors
    while( ! m_SoftIndicatorVariablesSelectors.isEmpty() ){
        VariableSelector* vs = m_SoftIndicatorVariablesSelectors.takeLast();
        ui->frmSoftIndicators->layout()->removeWidget( vs );
        vs->setParent( nullptr );
        delete vs;
    }
    //It is necessary to specify one soft indicator variable per c.d.f./p.d.f threshold/category
    //get the c.d.f./p.d.f. value pairs
    File* file = m_dfSelector->getSelectedFile();
    if( file ){
        file->readFromFS();
        int tot = file->getContentsCount();
        for( int i = 0; i < tot; ++i){
            VariableSelector* vs = new VariableSelector();
            ui->frmSoftIndicators->layout()->addWidget( vs );
            vs->onListVariables( m_psSoftSelector->getSelectedDataFile() );
            m_SoftIndicatorVariablesSelectors.append( vs );
        }
    }
}
