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
#include "domain/categorydefinition.h"
#include "domain/attribute.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "gslib/gslibparametersdialog.h"
#include "gslib/gslib.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "util.h"

#include <QInputDialog>
#include <QMessageBox>

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
    
    //set high-res icons for high-dpi displays
    if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI ){
        ui->btnConfigureAndRun->setIcon( QIcon(":icons32/setting32") );
        ui->btnSaveEstimates->setIcon( QIcon(":icons32/save32") );
        ui->btnFaciesMap->setIcon( QIcon(":icons32/faciesmap32") );
        ui->btnSaveGridForPostik->setIcon( QIcon(":icons32/save32") );
    }

    //configure actions specific for each variable type
    if( m_varType == IKVariableType::CATEGORICAL ){
        ui->lblSaveGridForPostik->hide();
        ui->btnSaveGridForPostik->hide();
    } else {
        ui->lblFaciesMap->hide();
        ui->btnFaciesMap->hide();
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

    //ik3d usually uses -9.9999 as no-data-value.
    m_cg_estimation->setNoDataValue( "-9.9999" );

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
    bool firstRun = false;

    //get the selected p.d.f./c.d.f. file
    File *distribution = m_dfSelector->getSelectedFile();
    if( ! distribution ){
        QMessageBox::critical( this, "Error", "Please, select a c.d.f./p.d.f. file.");
        return;
    }

    //get the selected point set data file
    PointSet *pointSet = (PointSet*)m_psSelector->getSelectedDataFile();
    if( ! pointSet ){
        QMessageBox::critical( this, "Error", "Please, select a point set file.");
        return;
    }

    //get the selected estimation grid
    CartesianGrid *cg = (CartesianGrid*)m_cgSelector->getSelectedDataFile();
    if( ! cg ){
        QMessageBox::critical( this, "Error", "Please, select an estimation grid.");
        return;
    }

    //-----------------------------set ik3d parameters---------------------------
    if( ! m_gpf_ik3d ){
        //create the parameters object
        m_gpf_ik3d = new GSLibParameterFile("ik3d");

        //set the default values, so we need to set fewer parameters here
        m_gpf_ik3d->setDefaultValues();

        //output file
        m_gpf_ik3d->getParameter<GSLibParFile*>(14)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");

        firstRun = true;
    }

    //-----------------these parameters must be re-set according to what the user has selected in the dialog--------

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
    min -= (min/10.0); //give 10% tolerance
    max += (max/10.0); //give 10% solerance

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
        m_gpf_ik3d->getParameter<GSLibParFile*>(9)->_path = "SOFT_INDICATOR_FILE_NOT_SET";
        GSLibParMultiValuedFixed *par10 = m_gpf_ik3d->getParameter<GSLibParMultiValuedFixed*>(10);
        par10->getParameter<GSLibParUInt*>(0)->_value = 0;
        par10->getParameter<GSLibParUInt*>(1)->_value = 0;
        par10->getParameter<GSLibParUInt*>(2)->_value = 0;
        GSLibParMultiValuedVariable *par10_3 = par10->getParameter<GSLibParMultiValuedVariable*>(3);
        //the soft data fields
        par10_3->setSize( ndist );
        for( uint i = 0; i < ndist; ++i){
            par10_3->getParameter<GSLibParUInt*>(i)->_value = 0;
        }
    }

    if( firstRun ){ //to not overwrite what the user might set
        //trimming limits
        GSLibParMultiValuedFixed *par11 = m_gpf_ik3d->getParameter<GSLibParMultiValuedFixed*>(11);
        par11->getParameter<GSLibParDouble*>(0)->_value = min;
        par11->getParameter<GSLibParDouble*>(1)->_value = max;
    }

    //set the grid parameters
    m_gpf_ik3d->setGridParameters( cg );

    //IK mode (has to specify threshold if mode is median IK)
    GSLibParMultiValuedFixed *par20 = m_gpf_ik3d->getParameter<GSLibParMultiValuedFixed*>(20);
    if( ui->radioFullIK->isChecked() )
        par20->getParameter<GSLibParOption*>(0)->_selected_value = 0;
    else
        par20->getParameter<GSLibParOption*>(0)->_selected_value = 1;

    //set the variogram model(s)
    bool selection_ok = true;
    GSLibParRepeat *par22 = m_gpf_ik3d->getParameter<GSLibParRepeat*>(22);
    if( ui->radioMedianIK->isChecked() ){ //median IK requires just one variogram model
        //this does not work.  Strangely ik3d still expects n-variograms even with median IK selected... figures.
        /*par22->setCount( 1 );
        GSLibParVModel *par22_0 = par22->getParameter<GSLibParVModel*>(0, 0);
        VariogramModelSelector* vms = m_variogramSelectors.at( 0 );
        VariogramModel *vmodel = vms->getSelectedVModel();
        par22_0->setFromVariogramModel( vmodel );*/

        //...repeating the same variogram model for each category/threshold...
        par22->setCount( ndist );
        for( uint i = 0; i < ndist; ++i){
            GSLibParVModel *par22_0 = par22->getParameter<GSLibParVModel*>(i, 0);
            VariogramModelSelector* vms = m_variogramSelectors.at( 0 );
            VariogramModel *vmodel = vms->getSelectedVModel();
            if( ! vmodel ){
                selection_ok = false;
                break;
            }
            par22_0->setFromVariogramModel( vmodel );
        }
    } else { //full IK requires one variogram model per c.d.f./p.d.f. threshold/class.
        par22->setCount( ndist );
        for( uint i = 0; i < ndist; ++i){
            GSLibParVModel *par22_0 = par22->getParameter<GSLibParVModel*>(i, 0);
            VariogramModelSelector* vms = m_variogramSelectors.at( i );
            VariogramModel *vmodel = vms->getSelectedVModel();
            if( ! vmodel ){
                selection_ok = false;
                break;
            }
            par22_0->setFromVariogramModel( vmodel );
        }
    }

    //if the variogram model selection is not ok...
    if( ! selection_ok ){
        //...delete the IK paramater set and return with an error message dialog.
        delete m_gpf_ik3d;
        m_gpf_ik3d = nullptr;
        QMessageBox::critical( this, "Error", "Please, select the necessary variogram(s) model(s).");
        return;
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

void IndicatorKrigingDialog::onSave()
{
    if( ! m_gpf_ik3d || ! m_cg_estimation ){
        QMessageBox::critical( this, "Error", "Please, run the estimation at least once.");
        return;
    }

    //get the selected estimation grid
    CartesianGrid* estimation_grid = (CartesianGrid*)m_cgSelector->getSelectedDataFile();
    if( ! estimation_grid ){
        QMessageBox::critical( this, "Error", "Please, select an estimation grid.");
        return;
    }

    if( m_varType == IKVariableType::CATEGORICAL ){
        //suggest a prefix for the variable names to the user
        QString prefix( "Probability_of_" );

        //presents a dialog so the user can change the suggested name.
        bool ok;
        prefix = QInputDialog::getText(this, "Define prefix for the variables",
                                                 "Prefix for the probability fields:", QLineEdit::Normal,
                                                 prefix, &ok);
        //if the user didn't cancel the input dialog
        if( ok ){
            //get the selected p.d.f. file
            CategoryPDF *pdf = (CategoryPDF *)m_dfSelector->getSelectedFile();

            //get the category definition used to create the p.d.f. (if defined)
            CategoryDefinition *cd = pdf->getCategoryDefinition();

            //for each code/probability pair
            for(int i = 0; i < pdf->getPairCount(); ++i){
                //make a meaningful name
                QString proposed_name( prefix );
                if( cd ){
                    cd->readFromFS(); //making sure the categorical information is loaded from the file.
                    proposed_name.append( cd->getCategoryName( i ) );
                } else {
                    Application::instance()->logWarn("IndicatorKrigingDialog::onSave(): null CategoryDefinition. Using "
                                                     "generic category names for the output variables.");
                    proposed_name.append( "Category_" ).append( pdf->get1stValue( i ) );
                }

                //the estimates normally follow the order of the categories in the resulting grid
                Attribute* values = m_cg_estimation->getAttributeFromGEOEASIndex( i + 1 );
                //add the estimates or variances to the selected estimation grid
                estimation_grid->addGEOEASColumn( values, proposed_name );
            }
        }
    }

    if( m_varType == IKVariableType::CONTINUOUS ){
        //suggest a prefix for the variable names to the user
        QString prefix( "Probability_below_" );

        //presents a dialog so the user can change the suggested name.
        bool ok;
        prefix = QInputDialog::getText(this, "Define prefix for the variables",
                                                 "Prefix for the cumulative probability fields:", QLineEdit::Normal,
                                                 prefix, &ok);
        //if the user didn't cancel the input dialog
        if( ok ){
            //get the selected c.d.f. file
            ThresholdCDF *cdf = (ThresholdCDF *)m_dfSelector->getSelectedFile();

            //for each code/probability pair
            for(int i = 0; i < cdf->getPairCount(); ++i){
                //make a meaningful name
                QString proposed_name( prefix );
                proposed_name.append( QString::number(cdf->get1stValue( i )) );

                //the estimates normally follow the order of the categories in the resulting grid
                Attribute* values = m_cg_estimation->getAttributeFromGEOEASIndex( i + 1 );
                //add the estimates or variances to the selected estimation grid
                estimation_grid->addGEOEASColumn( values, proposed_name );
            }
        }
    }
}

void IndicatorKrigingDialog::onCreateFaciesMap()
{
    if( ! m_gpf_ik3d || ! m_cg_estimation ){
        QMessageBox::critical( this, "Error", "Please, run the estimation at least once.");
        return;
    }

    //get the selected estimation grid
    CartesianGrid* estimation_grid = (CartesianGrid*)m_cgSelector->getSelectedDataFile();
    if( ! estimation_grid ){
        QMessageBox::critical( this, "Error", "Please, select an estimation grid.");
        return;
    }

    //get the selected p.d.f. file
    CategoryPDF *pdf = (CategoryPDF *)m_dfSelector->getSelectedFile();

    //define a temp path for the facies map
    QString faciesMapPath = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");

    //open the file for output
    QFile outputFile( faciesMapPath );
    outputFile.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&outputFile);

    //write cartesian grid header
    out << "Indicator Kriging facies map" << "\n";
    out << "1" << "\n";
    out << "Category" << "\n";

    //read the grid with the probability fields
    uint nLines = m_cg_estimation->getDataLineCount();
    uint nColumns = m_cg_estimation->getDataColumnCount();
    for( uint iLine = 0; iLine < nLines; ++iLine ){
        double maxLikelihood = -0.0001;
        int columnWithMaxLikelihood = -1;
        //find the column with the greatest value
        for( uint iColumn = 0; iColumn < nColumns; ++iColumn ){
            double prob = m_cg_estimation->data( iLine, iColumn );
            if( prob > maxLikelihood ){
                maxLikelihood = prob;
                columnWithMaxLikelihood = iColumn;
            }
        }
        //write the facies code
        if( columnWithMaxLikelihood >= 0 )
            out << pdf->get1stValue( columnWithMaxLikelihood ) << "\n";
        else
            out << "-1" << "\n";
    }

    //closes the output file
    outputFile.close();

    //create a CartesianGrid object from the recently created file
    CartesianGrid* cgFacies = new CartesianGrid( faciesMapPath );

    //set the grid paramaters with the same values of the estimation grid.
    cgFacies->setInfoFromOtherCG( m_cg_estimation );

    //the NDV for the facies grid is -1
    cgFacies->setNoDataValue( "-1" );

    //get the single variable
    Attribute* cat_var = (Attribute*)cgFacies->getChildByIndex( 0 );

    //get the category definition (if nullptr, the map will be displayed as a continuous variable)
    CategoryDefinition *cd = pdf->getCategoryDefinition();

    //open the plot dialog
    if( Util::viewGrid( cat_var, this, true, cd ) ){
        //make a meaningful name
        QString proposed_name;
        if( cd )
            proposed_name = cd->getName();
        else
            proposed_name = "Facies";

        //presents a dialog so the user can change the suggested name.
        bool ok;
        proposed_name = QInputDialog::getText(this, "Define variable name",
                                                 "Name for the categorical variable:", QLineEdit::Normal,
                                                 proposed_name, &ok);
        //if the user didn't cancel the input dialog
        if( ok ){
            //get the single variable
            Attribute* values = cgFacies->getAttributeFromGEOEASIndex( 1 );
            //add the categorical variable the selected estimation grid
            estimation_grid->addGEOEASColumn( values, proposed_name, true, cd );
        }
    }
}

void IndicatorKrigingDialog::onSaveForPostik()
{
    if( ! m_gpf_ik3d || ! m_cg_estimation ){
        QMessageBox::critical( this, "Error", "Please, run the estimation at least once.");
        return;
    }

    //get the selected estimation grid
    CartesianGrid* estimation_grid = (CartesianGrid*)m_cgSelector->getSelectedDataFile();
    if( ! estimation_grid ){
        QMessageBox::critical( this, "Error", "Please, select an estimation grid to set the new grid geometry from.");
        return;
    }

    //postik is not available for categorical variables
    if( m_varType == IKVariableType::CATEGORICAL ){
        QMessageBox::critical( this, "Error", "Post-processing is not available for categorical variables.");
        return;
    }

    //get the selected estimation grid
    CartesianGrid *selectedCG = (CartesianGrid*)m_cgSelector->getSelectedDataFile();

    //presents a naming dialog with a suggested name for the new Cartesian grid.
    bool ok;
    QString new_cg_name = m_PointSetVariableSelector->getSelectedVariableName() + "_IK_estimates_for_PostIK";
    new_cg_name = QInputDialog::getText(this, "Name the new cartesian grid",
                                             "Name for the new Cartesian grid:", QLineEdit::Normal,
                                             new_cg_name, &ok);

    //if the user didn't cancel the input dialog
    if( ok ){
        //get the IK output file
        QString tmp_file_path = m_gpf_ik3d->getParameter<GSLibParFile*>(14)->_path;

        //create a new grid object corresponding to the file with the IK estimates
        CartesianGrid* cg = new CartesianGrid( tmp_file_path );

        //set the metadata info from the estimation grid selected by the user
        cg->setInfoFromOtherCG( selectedCG, false );

        //import the grid file with the IK estimates as a project item
        Application::instance()->getProject()->importCartesianGrid( cg, new_cg_name );
    }
}
