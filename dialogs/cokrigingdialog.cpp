#include "cokrigingdialog.h"
#include "ui_cokrigingdialog.h"
#include "domain/application.h"
#include "domain/pointset.h"
#include "domain/cartesiangrid.h"
#include "domain/project.h"
#include "domain/variogrammodel.h"
#include "widgets/pointsetselector.h"
#include "widgets/variableselector.h"
#include "widgets/cartesiangridselector.h"
#include "widgets/variogrammodelselector.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslibparametersdialog.h"
#include "gslib/gslib.h"
#include "util.h"

#include <QFile>
#include <QInputDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <limits>
#include <tuple>

CokrigingDialog::CokrigingDialog(QWidget *parent, CokrigingProgram cokProg) :
    QDialog(parent),
    ui(new Ui::CokrigingDialog),
    m_gpf_cokb3d( nullptr ),
    m_cg_estimation( nullptr ),
    m_cokProg( cokProg ),
    m_newcokb3dModelType( CokrigingModelType::MM2 ),
    m_secVarForMM2Selector( nullptr )
{

    ui->setupUi(this);

    if( cokProg == CokrigingProgram::COKB3D )
    {
        this->setWindowTitle("Cokriging (with cokb3d)");
        ui->frmOuterSecondaryData->setVisible( false );
        ui->frmOuterLVMData->setVisible( false );
        ui->frmModelType->setVisible( false );
    }else{
        this->setWindowTitle("Cokriging (with newcokb3d)");
        ui->frmOuterSecondaryData->setVisible( true );
        ui->frmOuterLVMData->setVisible( true );
        ui->frmModelType->setVisible( true );
    }

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    //The list with existing point sets in the project for the input data.
    m_psInputSelector = new PointSetSelector();
    ui->frmData->layout()->addWidget( m_psInputSelector );
    QFont font = m_psInputSelector->font();
    font.setBold( false );
    m_psInputSelector->setFont( font );

    //The list for primary variable selection.
    m_inputPrimVarSelector = makeVariableSelector();
    ui->frmData->layout()->addWidget( m_inputPrimVarSelector );
    m_inputPrimVarSelector->setFont( font );
    connect( m_psInputSelector, SIGNAL(pointSetSelected(DataFile*)),
             m_inputPrimVarSelector, SLOT(onListVariables(DataFile*)) );
    connect( m_inputPrimVarSelector, SIGNAL(variableSelected(Attribute*)),
             this, SLOT(onUpdateVarMatrixLabels()));

    //The list with existing cartesian grids in the project for the estimation.
    m_cgEstimationGridSelector = new CartesianGridSelector();
    ui->frmGrid->layout()->addWidget( m_cgEstimationGridSelector );
    font = m_cgEstimationGridSelector->font();
    font.setBold( false );
    m_cgEstimationGridSelector->setFont( font );

    //The list with existing cartesian grids in the project for the secondary data.
    m_cgSecondaryGridSelector = new CartesianGridSelector( true );
    ui->frmSecondaryData->layout()->addWidget( m_cgSecondaryGridSelector );
    font = m_cgSecondaryGridSelector->font();
    font.setBold( false );
    m_cgSecondaryGridSelector->setFont( font );

    //The list with existing Cartesian grids in the project for the locally varying mean.
    m_cgLVMGridSelector = new CartesianGridSelector( true );
    ui->frmLVMData->layout()->addWidget( m_cgLVMGridSelector );
    font = m_cgLVMGridSelector->font();
    font.setBold( false );
    m_cgLVMGridSelector->setFont( font );

    //The combo box of secondary variables available for the MM2 model (newcokb3d)
    m_secVarForMM2Selector = makeVariableSelector();
    ui->frmModelType->layout()->addWidget( m_secVarForMM2Selector );

    //Call this slot to create the widgets that are function of the number of secondary variables
    onNumberOfSecondaryVariablesChanged( 1 );

    //calling this slot causes the variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_psInputSelector->onSelection( 0 );
    m_cgSecondaryGridSelector->onSelection( 0 );

    onModelTypeChanged();
}

CokrigingDialog::~CokrigingDialog()
{
    delete ui;
    Application::instance()->logInfo("CokrigingDialog destroyed.");
}

void CokrigingDialog::onNumberOfSecondaryVariablesChanged(int n)
{
    //clears the current set of secondary variable selectors
    QVector<VariableSelector*>::iterator it = m_inputSecVarsSelectors.begin();
    for(; it != m_inputSecVarsSelectors.end(); ++it){
        delete (*it);
    }
    m_inputSecVarsSelectors.clear();

    //installs the target number of secondary variable selectors
    for( int i = 0; i < n; ++i){
        VariableSelector* selector = makeVariableSelector();
        ui->frmData->layout()->addWidget( selector );
        connect( m_psInputSelector, SIGNAL(pointSetSelected(DataFile*)),
                 selector, SLOT(onListVariables(DataFile*)) );
        connect( selector, SIGNAL(variableSelected(Attribute*)),
                 this, SLOT(onUpdateVarMatrixLabels()));
        m_inputSecVarsSelectors.push_back( selector );
    }
    //--------------------------------------------------------------------

    //clears the current set of secondary grid variable selectors
    it = m_inputGridSecVarsSelectors.begin();
    for(; it != m_inputGridSecVarsSelectors.end(); ++it){
        delete (*it);
    }
    m_inputGridSecVarsSelectors.clear();

    //installs the target number of secondary grid variable selectors
    // forcing to be one.  In the future, cokriging programs may accept more than one.
    for( int i = 0; i < 1/*n*/; ++i){
        VariableSelector* selector = makeVariableSelector();
        ui->frmSecondaryData->layout()->addWidget( selector );
        connect( m_cgSecondaryGridSelector, SIGNAL(cartesianGridSelected(DataFile*)),
                 selector, SLOT(onListVariables(DataFile*)) );
        m_inputGridSecVarsSelectors.push_back( selector );
    }
    //---------------------------------------------------------------------

    //clears the current set of grid LVM variable selectors
    it = m_inputLVMVarsSelectors.begin();
    for(; it != m_inputLVMVarsSelectors.end(); ++it){
        delete (*it);
    }
    m_inputLVMVarsSelectors.clear();

    //installs the target number of LVM grid variable selectors
    // forcing to be one.  In the future, cokriging programs may accept more than one.
    for( int i = 0; i < 1/*n*/; ++i){
        VariableSelector* selector = makeVariableSelector();
        ui->frmLVMData->layout()->addWidget( selector );
        connect( m_cgLVMGridSelector, SIGNAL(cartesianGridSelected(DataFile*)),
                 selector, SLOT(onListVariables(DataFile*)) );
        m_inputLVMVarsSelectors.push_back( selector );
    }
    //---------------------------------------------------------------------


    //clears the current set of lebels in the variography matrix top bar
    QVector<QLabel*>::iterator itLabels = m_labelsVarMatrixTopHeader.begin();
    for(; itLabels != m_labelsVarMatrixTopHeader.end(); ++itLabels){
        delete (*itLabels);
    }
    m_labelsVarMatrixTopHeader.clear();
    //---------------------------------------------------------------------

    //clears the current set of lebels in the variography matrix left bar
    itLabels = m_labelsVarMatrixLeftHeader.begin();
    for(; itLabels != m_labelsVarMatrixLeftHeader.end(); ++itLabels){
        delete (*itLabels);
    }
    m_labelsVarMatrixLeftHeader.clear();
    //---------------------------------------------------------------------

    //calling this slot causes the variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_psInputSelector->onSelection( 0 );
    m_cgSecondaryGridSelector->onSelection( 0 );

    //update the table of variogram widgets
    onUpdateVariogramMatrix( n );

    onModelTypeChanged();
}

void CokrigingDialog::onUpdateVariogramMatrix( int numberOfSecondaryVariables )
{
    //get the pointer to the grid layout
    QGridLayout* layout = (QGridLayout*)ui->frmVariogramArray->layout();

    //clean the current widgets
    while (QLayoutItem* item = layout->takeAt( 0 ))
       delete item->widget();

    //clean the lists of created widgets
    m_labelsVarMatrixTopHeader.clear();
    m_labelsVarMatrixLeftHeader.clear();
    m_variograms.clear();

    //place holder at row=0, col=0
    layout->addWidget( new QLabel(), 0, 0, 1, 1 );

    //label for the primary variable in the table top header
    m_labelsVarMatrixTopHeader.append( makeLabel( m_inputPrimVarSelector->getSelectedVariableName() ) );
    layout->addWidget( m_labelsVarMatrixTopHeader.last() , 0, 2, 1, 1 );
    //label for the first secondary variable in the table top header
    m_labelsVarMatrixTopHeader.append( makeLabel( m_inputSecVarsSelectors[0]->getSelectedVariableName() ) );
    layout->addWidget(  m_labelsVarMatrixTopHeader.last(), 0, 3, 1, 1 );

    //separator under the table header
    layout->addWidget( Util::createHorizontalLine(), 1, 0, 1, 3 + numberOfSecondaryVariables );

    //label for the primary variable in the table left header
    m_labelsVarMatrixLeftHeader.append( makeLabel( m_inputPrimVarSelector->getSelectedVariableName() ) );
    layout->addWidget( m_labelsVarMatrixLeftHeader.last(), 2, 0, 1, 1 );
    //label for the first secondary variable in the table left header
    m_labelsVarMatrixLeftHeader.append( makeLabel( m_inputSecVarsSelectors[0]->getSelectedVariableName() ) );
    layout->addWidget( m_labelsVarMatrixLeftHeader.last(), 3, 0, 1, 1 );

    //separator after the table left header
    layout->addWidget( Util::createVerticalLine(), 0, 1, 3 + numberOfSecondaryVariables, 1 );

    //labels for possibly further secondary variables
    for( int i = 1; i < numberOfSecondaryVariables; ++i){
        m_labelsVarMatrixTopHeader.append( makeLabel( m_inputSecVarsSelectors[i]->getSelectedVariableName() ) );
        layout->addWidget( m_labelsVarMatrixTopHeader.last(), 0, 3+i, 1, 1 );
         m_labelsVarMatrixLeftHeader.append( makeLabel( m_inputSecVarsSelectors[i]->getSelectedVariableName() ) );
        layout->addWidget( m_labelsVarMatrixLeftHeader.last() , 3+i, 0, 1, 1 );
    }

    //The list with existing variogram models for the primary variable.
    VariogramModelSelector* vModelSelector = makeVariogramModelSelector();
    layout->addWidget( vModelSelector, 2, 2, 1, 1 );
    m_variograms.append( std::make_tuple(1,1,vModelSelector) ); //autovariogram for the primary

    //The list with existing variogram models for the 1st secondary variable.
    vModelSelector = makeVariogramModelSelector();
    layout->addWidget( vModelSelector, 3, 3, 1, 1 );
    m_variograms.append( std::make_tuple(2,2,vModelSelector) ); //autovariogram for the 1st secondary

    //The list with existing variogram models for the primary/1st secondary cross variography.
    vModelSelector = makeVariogramModelSelector();
    layout->addWidget( vModelSelector, 2, 3, 1, 1 );
    m_variograms.append( std::make_tuple(1,2,vModelSelector) ); //cross variogram primary->1st secondary

    //Lists of variogram models for possibly further secondary variables
    for( int i = 1; i < numberOfSecondaryVariables; ++i){
        for( int j = 0; j < i + 2; ++j){
            vModelSelector = makeVariogramModelSelector();
            layout->addWidget( vModelSelector, 2+j, 3+i, 1, 1 );
            m_variograms.append( std::make_tuple(1+j,2+i,vModelSelector) ); //auto/cross variograms for other combinations of variables
        }
    }
}

void CokrigingDialog::onUpdateVarMatrixLabels()
{
    //must have at least one primary and one secondary variable
    if( m_labelsVarMatrixTopHeader.size() < 2 )
        return;

    QVector<QLabel*>::iterator itLabels = m_labelsVarMatrixTopHeader.begin();
    QVector<QLabel*>::iterator itLeftLabels = m_labelsVarMatrixLeftHeader.begin();

    //first label is that for the primary variable
    (*itLabels)->setText( m_inputPrimVarSelector->getSelectedVariableName() );
    (*itLeftLabels)->setText( m_inputPrimVarSelector->getSelectedVariableName() );
    ++itLabels;
    ++itLeftLabels;

    //update the labels for the secondary variables
    QVector<VariableSelector*>::iterator itSelectors = m_inputSecVarsSelectors.begin();
    for(; itSelectors != m_inputSecVarsSelectors.end(); ++itLabels, ++itLeftLabels, ++itSelectors){
        (*itLabels)->setText( (*itSelectors)->getSelectedVariableName() );
        (*itLeftLabels)->setText( (*itSelectors)->getSelectedVariableName() );
    }
}

void CokrigingDialog::onParameters()
{
    //surely the selected data is a PointSet
    PointSet* psInputData = (PointSet*)m_psInputSelector->getSelectedDataFile();

    if( ! psInputData ){
        QMessageBox::critical( this, "Error", "Please, select a point set data file.");
        return;
    }

    //get the selected estimation grid
    CartesianGrid* estimation_grid = (CartesianGrid*)m_cgEstimationGridSelector->getSelectedDataFile();
    if( ! estimation_grid ){
        QMessageBox::critical( this, "Error", "Please, select an estimation grid.");
        return;
    }

    //surely the co-located secondary is in a CartesianGrid
    CartesianGrid* cgColocSec = (CartesianGrid*)m_cgSecondaryGridSelector->getSelectedDataFile();

    //-----------determine the absolute maximum and minimum of input data in all the selected variables-----------
    //load the data
    psInputData->loadData();
    //init max with min and min with max ;-)
    double minAll = std::numeric_limits<double>::max();
    double maxAll = std::numeric_limits<double>::min();
    //gather the indexes of the selected variables
    QVector<int> selectedColumns;
    selectedColumns.append( m_inputPrimVarSelector->getSelectedVariableGEOEASIndex() - 1 );
    for( uint i = 0; i < (uint)m_inputSecVarsSelectors.size(); ++i){
        selectedColumns.append( m_inputSecVarsSelectors[i]->getSelectedVariableGEOEASIndex() - 1 );
    }
    //compute the absolute max and min from the MINs and MAXes of each variables
    QVector<int>::iterator it = selectedColumns.begin();
    for(; it != selectedColumns.end(); ++it ){
        double min = psInputData->min( *it );
        double max = psInputData->max( *it );
        if( min < minAll )
            minAll = min;
        if( max > maxAll )
            maxAll = max;
    }
    //--------------------------------------------------------------------------------------------------------------

    //get the number of variables (primary + secondaries)
    uint nvars = 1 + ui->spinNSecVars->value();

    //-----------------------------set cokb3d parameters---------------------------
    if( ! m_gpf_cokb3d ){
        //create the parameters object
        m_gpf_cokb3d = new GSLibParameterFile("cokb3d");

        //set the default values, so we need to set fewer parameters here
        m_gpf_cokb3d->setDefaultValues();

        //trimming limits
        GSLibParMultiValuedFixed *par3 = m_gpf_cokb3d->getParameter<GSLibParMultiValuedFixed*>(3);
        par3->getParameter<GSLibParDouble*>(0)->_value = minAll;
        par3->getParameter<GSLibParDouble*>(1)->_value = maxAll;

        //file with estimates output
        m_gpf_cokb3d->getParameter<GSLibParFile*>(9)->_path =
                Application::instance()->getProject()->generateUniqueTmpFilePath("out");

        // maximum search radii for primary (init from variogram ranges)
        VariogramModel* primVariogram = getVariogramModel(1, 1);
        GSLibParMultiValuedFixed *par13 = m_gpf_cokb3d->getParameter<GSLibParMultiValuedFixed*>(13);
        par13->getParameter<GSLibParDouble*>(0)->_value = primVariogram->get_max_hMax();
        par13->getParameter<GSLibParDouble*>(1)->_value = primVariogram->get_max_hMin();
        par13->getParameter<GSLibParDouble*>(2)->_value = primVariogram->get_max_vert();

        //maximum search radii for secondaries (init from variogram ranges)
        double max_a_hMax = -1.0;
        double max_a_hMin = -1.0;
        double max_a_vert = -1.0;
        for( uint tail = 1; tail <= nvars; ++tail)
            for( uint head = 2; head <= nvars; ++head){
                VariogramModel *vm = getVariogramModel( head, tail );
                if( max_a_hMax < vm->get_max_hMax() )
                    max_a_hMax = vm->get_max_hMax();
                if( max_a_hMin < vm->get_max_hMin() )
                    max_a_hMin = vm->get_max_hMin();
                if( max_a_vert < vm->get_max_vert() )
                    max_a_vert = vm->get_max_vert();
            }
        GSLibParMultiValuedFixed *par14 = m_gpf_cokb3d->getParameter<GSLibParMultiValuedFixed*>(14);
        par14->getParameter<GSLibParDouble*>(0)->_value = max_a_hMax;
        par14->getParameter<GSLibParDouble*>(1)->_value = max_a_hMin;
        par14->getParameter<GSLibParDouble*>(2)->_value = max_a_vert;

        //angles for search ellipsoid (init from the last nested structure of the primary variogram)
        GSLibParMultiValuedFixed *par15 = m_gpf_cokb3d->getParameter<GSLibParMultiValuedFixed*>(15);
        par15->getParameter<GSLibParDouble*>(0)->_value = primVariogram->getAzimuth( primVariogram->getNst()-1 );
        par15->getParameter<GSLibParDouble*>(1)->_value = primVariogram->getDip( primVariogram->getNst()-1 );
        par15->getParameter<GSLibParDouble*>(2)->_value = primVariogram->getRoll( primVariogram->getNst()-1 );

        //means (primary and secondaries)
        GSLibParMultiValuedVariable *par17 = m_gpf_cokb3d->getParameter<GSLibParMultiValuedVariable*>(17);
        par17->assure( nvars );
        QVector<int>::iterator it = selectedColumns.begin();
        for(uint i = 0; it != selectedColumns.end(); ++it, ++i ){
            par17->getParameter<GSLibParDouble*>( i )->_value = psInputData->mean( *it );
        }
    }

    //input data file
    m_gpf_cokb3d->getParameter<GSLibParFile*>(0)->_path = psInputData->getPath();

    //number of variables (primary + secondaries)
    m_gpf_cokb3d->getParameter<GSLibParUInt*>(1)->_value = nvars;

    //columns for X,Y,Z,primary and secondaries
    GSLibParMultiValuedVariable *par2 = m_gpf_cokb3d->getParameter<GSLibParMultiValuedVariable*>(2);
    par2->assure( 3 + nvars );
    par2->getParameter<GSLibParUInt*>(0)->_value = psInputData->getXindex();
    par2->getParameter<GSLibParUInt*>(1)->_value = psInputData->getYindex();
    par2->getParameter<GSLibParUInt*>(2)->_value = psInputData->getZindex();
    par2->getParameter<GSLibParUInt*>(3)->_value = m_inputPrimVarSelector->getSelectedVariableGEOEASIndex();
    for( uint i = 0; i < (uint)m_inputSecVarsSelectors.size(); ++i){
        par2->getParameter<GSLibParUInt*>(4 + i)->_value = m_inputSecVarsSelectors[i]->getSelectedVariableGEOEASIndex();
    }

    if( cgColocSec ){
        //cokriging type (co-located)
        m_gpf_cokb3d->getParameter<GSLibParOption*>(4)->_selected_value = 1;
        //file with co-located secondary data
        m_gpf_cokb3d->getParameter<GSLibParFile*>(5)->_path = cgColocSec->getPath();
        //column with co-located secondary data
        m_gpf_cokb3d->getParameter<GSLibParUInt*>(6)->_value = m_inputGridSecVarsSelectors[0]->getSelectedVariableGEOEASIndex();
    }else{
        //cokriging type (full)
        m_gpf_cokb3d->getParameter<GSLibParOption*>(4)->_selected_value = 0;
        //file with co-located secondary data
        m_gpf_cokb3d->getParameter<GSLibParFile*>(5)->_path = "";
        //column with co-located secondary data
        m_gpf_cokb3d->getParameter<GSLibParUInt*>(6)->_value = 0;
    }

    //grid parameters
    GSLibParGrid* par10 = m_gpf_cokb3d->getParameter<GSLibParGrid*>(10);
    par10->_specs_x->getParameter<GSLibParUInt*>(0)->_value = estimation_grid->getNX(); //nx
    par10->_specs_x->getParameter<GSLibParDouble*>(1)->_value = estimation_grid->getX0(); //min x
    par10->_specs_x->getParameter<GSLibParDouble*>(2)->_value = estimation_grid->getDX(); //cell size x
    par10->_specs_y->getParameter<GSLibParUInt*>(0)->_value = estimation_grid->getNY(); //ny
    par10->_specs_y->getParameter<GSLibParDouble*>(1)->_value = estimation_grid->getY0(); //min y
    par10->_specs_y->getParameter<GSLibParDouble*>(2)->_value = estimation_grid->getDY(); //cell size y
    par10->_specs_z->getParameter<GSLibParUInt*>(0)->_value = estimation_grid->getNZ(); //nz
    par10->_specs_z->getParameter<GSLibParDouble*>(1)->_value = estimation_grid->getZ0(); //min z
    par10->_specs_z->getParameter<GSLibParDouble*>(2)->_value = estimation_grid->getDZ(); //cell size z

    //-------------------------------------------------auto and cross variograms-------------------------//
    GSLibParRepeat *par18 = m_gpf_cokb3d->getParameter<GSLibParRepeat*>(18);
    par18->setCount( m_variograms.count() );
    QVector< std::tuple<uint,uint,VariogramModelSelector*> >::iterator itVariogram = m_variograms.begin();
    for(uint i = 0; itVariogram != m_variograms.end(); ++itVariogram, ++i){
        std::tuple<uint,uint,VariogramModelSelector*> tuple = *itVariogram;
        uint head = std::get<0>( tuple );
        uint tail = std::get<1>( tuple );
        //variable indexes
        GSLibParMultiValuedFixed *par18_ii = par18->getParameter<GSLibParMultiValuedFixed*>(i, 0);
        par18_ii->getParameter<GSLibParUInt*>(0)->_value = head;
        par18_ii->getParameter<GSLibParUInt*>(1)->_value = tail;
        //variogram model
        VariogramModelSelector *vms = std::get<2>( tuple );
        VariogramModel *vm = vms->getSelectedVModel();
        GSLibParVModel *par18_i = par18->getParameter<GSLibParVModel*>(i, 1);
        par18_i->setFromVariogramModel( vm );
    }
    //-------------------------------------------------------------------------------------------------------

    //----------------------------prepare and execute cokb3d--------------------------------

    //show the cokb3d parameters
    GSLibParametersDialog gsd( m_gpf_cokb3d, this );
    int result = gsd.exec();

    //if user didn't cancel the dialog
    if( result == QDialog::Accepted ){
        //Generate the parameter file
        QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "par" );
        m_gpf_cokb3d->save( par_file_path );

        //to be notified when cokb3d completes.
        connect( GSLib::instance(), SIGNAL(programFinished()), this, SLOT(onCokb3dCompletes()) );

        //run cokb3d program asynchronously
        Application::instance()->logInfo("Starting cokb3d program...");
        GSLib::instance()->runProgramAsync( "cokb3d", par_file_path );
    }
}

void CokrigingDialog::onLMCcheck()
{
    Application::instance()->logWarningOff();
    bool result = true;
    //get the number of variables (primary + secondaries)
    uint nvars = 1 + ui->spinNSecVars->value();
    for( uint i = 1; i < nvars; ++i)
        for( uint j = i + 1; j <= nvars; ++j){
            VariogramModel *autoVar1 = getVariogramModel( i, i );
            VariogramModel *autoVar2 = getVariogramModel( j, j );
            VariogramModel *crossVar = getVariogramModel( i, j );
            result = Util::isLMC( autoVar1, autoVar2, crossVar );
        }
    if( ! result ){
        QMessageBox::critical( this, "Error", "The variograms do not form a LMC.  Please, check the message panel for error messages with the details.");
    }
    Application::instance()->logWarningOn();
}

void CokrigingDialog::onCokb3dCompletes()
{
    //frees all signal connections to the GSLib singleton.
    GSLib::instance()->disconnect();

    preview();
}

void CokrigingDialog::onSave()
{
    save( true );
}

void CokrigingDialog::onSaveKrigingVariances()
{
    save( false );
}

void CokrigingDialog::onModelTypeChanged()
{
    //do nothing the the cokriging program is not newcokb3d
    if( m_cokProg != CokrigingProgram::NEWCOKB3D )
        return;
    //determine variography model type
    int index = ui->cmbModelType->currentIndex();
    switch( index ){
        case 0: m_newcokb3dModelType = CokrigingModelType::MM1; break;
        case 1: m_newcokb3dModelType = CokrigingModelType::MM2; break;
        case 2: m_newcokb3dModelType = CokrigingModelType::LMC; break;
    }
    //for all current variogram selectors
    QVector< std::tuple<uint,uint,VariogramModelSelector*> >::iterator it = m_variograms.begin();
    for(; it != m_variograms.end(); ++it){
        std::tuple<uint,uint,VariogramModelSelector*> tuple = *it;
        uint head = std::get<0>( tuple );
        uint tail = std::get<1>( tuple );
        VariogramModelSelector* vModelSelector = std::get<2>( tuple );
        //diable all variogram selectors a priori
        vModelSelector->setEnabled( false );
        //if MM1, then enable the autovariogram for primary
        if( m_newcokb3dModelType == CokrigingModelType::MM1 && head == 1 && tail == 1 )
            vModelSelector->setEnabled( true );
        //if MM2, then enable the autovariograms for the secondaries
        if( m_newcokb3dModelType == CokrigingModelType::MM2 && head == tail && head > 1 )
            vModelSelector->setEnabled( true );
        //if LMC, then enable all variograms (auto- and cross-)
        if( m_newcokb3dModelType == CokrigingModelType::LMC )
            vModelSelector->setEnabled( true );
    }
    //enable the secondary variable selector for MM2
    if( m_newcokb3dModelType == CokrigingModelType::MM2 ){
        ui->lblSecVarForMM2->setEnabled( true );
        if( m_secVarForMM2Selector )
            m_secVarForMM2Selector->setEnabled( true );
    } else {
        ui->lblSecVarForMM2->setEnabled( false );
        if( m_secVarForMM2Selector )
            m_secVarForMM2Selector->setEnabled( false );
    }
}

void CokrigingDialog::preview()
{
    if( m_cg_estimation )
        delete m_cg_estimation;

    //get the tmp file path created by cokb3d with the estimates and kriging variances
    QString grid_file_path = m_gpf_cokb3d->getParameter<GSLibParFile*>(9)->_path;

    //cokb3d may fail to estimate, most of times due to non-LMC variography
    QFile file( grid_file_path );
    if( ! file.exists() ){
        Application::instance()->logError( "File with estimates not found. Check cokb3d messages." );
        return;
    }

    //create a new grid object corresponding to the file created by cokb3d
    m_cg_estimation = new CartesianGrid( grid_file_path );

    //set the grid geometry info.
    m_cg_estimation->setInfoFromGridParameter( m_gpf_cokb3d->getParameter<GSLibParGrid*>(10) );

    //cokb3d usually uses -999 as no-data-value
    m_cg_estimation->setNoDataValue( "-999" );

    //get the variable with the estimation values (normally the first)
    Attribute* est_var = (Attribute*)m_cg_estimation->getChildByIndex( 0 );

    //open the plot dialog
    Util::viewGrid( est_var, this );
}

void CokrigingDialog::save(bool estimates)
{
    if( ! m_gpf_cokb3d || ! m_cg_estimation ){
        QMessageBox::critical( this, "Error", "Please, run the estimation at least once.");
        return;
    }

    //get the selected estimation grid
    CartesianGrid* estimation_grid = (CartesianGrid*)m_cgEstimationGridSelector->getSelectedDataFile();
    if( ! estimation_grid ){
        QMessageBox::critical( this, "Error", "Please, select an estimation grid.");
        return;
    }

    //suggest a name to the user
    QString proposed_name( m_inputPrimVarSelector->getSelectedVariableName() );
    proposed_name = proposed_name.append( ( estimates ? "_CK_ESTIMATES" : "_CK_KVARIANCES" ) );

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


QLabel *CokrigingDialog::makeLabel(const QString caption)
{
    QLabel* label = new QLabel( caption );
    label->setStyleSheet("font-weight: normal;");
    label->setAlignment( Qt::AlignHCenter );
    return label;
}

VariableSelector *CokrigingDialog::makeVariableSelector()
{
    VariableSelector* vs = new VariableSelector( );
    vs->setStyleSheet("font-weight: normal;");
    return vs;
}

VariogramModelSelector *CokrigingDialog::makeVariogramModelSelector()
{
    VariogramModelSelector* vs = new VariogramModelSelector( );
    vs->setStyleSheet("font-weight: normal;");
    return vs;
}

VariogramModel *CokrigingDialog::getVariogramModel(uint head, uint tail)
{
    VariogramModel* result = nullptr;
    QVector< std::tuple<uint,uint,VariogramModelSelector*> >::iterator it = m_variograms.begin();
    for(; it != m_variograms.end(); ++it){
        std::tuple<uint,uint,VariogramModelSelector*> tuple = *it;
        uint myHead = std::get<0>( tuple );
        uint myTail = std::get<1>( tuple );
        if( ( myHead == head && myTail == tail ) ||
            ( myHead == tail && myTail == head ) ){
            VariogramModelSelector* vModelSelector = std::get<2>( tuple );
            return vModelSelector->getSelectedVModel();
        }
    }
    return result;
}
