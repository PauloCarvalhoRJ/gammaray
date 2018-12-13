#include "creategriddialog.h"
#include "ui_creategriddialog.h"
#include "domain/pointset.h"
#include "domain/cartesiangrid.h"
#include "widgets/variogrammodellist.h"
#include "domain/application.h"
#include "domain/project.h"
#include "domain/attribute.h"
#include "gslib/gslibparams/widgets/widgetgslibpargrid.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "util.h"
#include <QInputDialog>
#include <math.h>
#include <QMessageBox>
#include <QDir>
#include <QRegularExpression>
#include "domain/variogrammodel.h"
#include "gslib/gslib.h"

CreateGridDialog::CreateGridDialog(PointSet *pointSet, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateGridDialog),
    m_pointSet( pointSet )
{
    ui->setupUi(this);
    this->setWindowTitle( "Create estimation/simulation grid for " + m_pointSet->getName() );

    //The list with existing variogram models in the project.
    m_vModelList = new VariogramModelList();
    ui->frmVariogramModels->layout()->addWidget( m_vModelList );

    //suggest grid parameters from the point set bounding box
    pointSet->loadData();
    double xmin = pointSet->min( pointSet->getXindex()-1 );
    double xmax = pointSet->max( pointSet->getXindex()-1 );
    double ymin = pointSet->min( pointSet->getYindex()-1 );
    double ymax = pointSet->max( pointSet->getYindex()-1 );
    double zmin;
    double zmax;
    double deltaX =  fabs(xmax - xmin);
    double deltaY =  fabs(ymax - ymin);
    double deltaZ;
    //sometimes point sets with all X,Y,Z coordinates defined are actually 2D
    if( pointSet->is3D() ){
        zmin = pointSet->min( pointSet->getZindex()-1 );
        zmax = pointSet->max( pointSet->getZindex()-1 );
        deltaZ = fabs(zmax - zmin);
    } else {
        zmin = 0.0;
        zmax = 0.0;
        deltaZ = 1.0;
    }
    bool isFlat = ( !pointSet->is3D() || Util::almostEqual2sComplement( zmin, zmax, 1 ) );
    //The grid parameters widget (reusing the one orginially made for the GSLib parameter dialog).
    m_par = new GSLibParGrid("", "", "");
    m_par->_specs_x->getParameter<GSLibParUInt*>(0)->_value = 100+1; //nx
    m_par->_specs_x->getParameter<GSLibParDouble*>(1)->_value = xmin; //min x
    m_par->_specs_x->getParameter<GSLibParDouble*>(2)->_value = deltaX / 100.0; //cell size x
    m_par->_specs_y->getParameter<GSLibParUInt*>(0)->_value = 100+1; //ny
    m_par->_specs_y->getParameter<GSLibParDouble*>(1)->_value = ymin; //min y
    m_par->_specs_y->getParameter<GSLibParDouble*>(2)->_value = deltaY / 100.0; //cell size y
    m_par->_specs_z->getParameter<GSLibParUInt*>(0)->_value = (isFlat ? 1 : (10+1)); //nz
    m_par->_specs_z->getParameter<GSLibParDouble*>(1)->_value = zmin; //min z
    m_par->_specs_z->getParameter<GSLibParDouble*>(2)->_value = (isFlat ? 1.0 : (deltaZ / 10.0)); //cell size z
    m_gridParameters = new WidgetGSLibParGrid();
    ui->grpGridParameters->layout()->addWidget( m_gridParameters );
    m_gridParameters->fillFields( m_par );

    connect( m_vModelList, SIGNAL(variogramClicked()), this, SLOT(onVariogramClicked()));

    if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI ){
        ui->btnRunGammaBar->setIcon( QIcon(":icons32/gbar32") );
    }

    adjustSize();
}

CreateGridDialog::~CreateGridDialog()
{
    delete ui;
    delete m_vModelList;
    delete m_par;
    delete m_gridParameters;
    Application::instance()->logInfo("Create grid dialog destroyed.");
}

void CreateGridDialog::runGammaBar()
{
    //read values entered by the user
    m_gridParameters->updateValue( m_par );
    //get selected variogram model
    VariogramModel* vm = m_vModelList->getSelectedVModel();
    //makes sure user selected one model
    if( !vm ){
        QMessageBox::critical( this, "Error", "Please, select one variogram model.");
        return;
    }

    GSLibParameterFile gpar("gammabar");
    //set variogram model parameters
    gpar.setVariogramModel( vm );
    //set grid cell sizes
    GSLibParMultiValuedFixed *par0 = gpar.getParameter<GSLibParMultiValuedFixed*>(0);
    par0->getParameter<GSLibParDouble*>(0)->_value = m_par->_specs_x->getParameter<GSLibParDouble*>(2)->_value;
    par0->getParameter<GSLibParDouble*>(1)->_value = m_par->_specs_y->getParameter<GSLibParDouble*>(2)->_value;
    par0->getParameter<GSLibParDouble*>(2)->_value = m_par->_specs_z->getParameter<GSLibParDouble*>(2)->_value;
    //leave block discretization as one
    GSLibParMultiValuedFixed *par1 = gpar.getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = ui->txtBlkDiscrX->text().toInt();
    par1->getParameter<GSLibParUInt*>(1)->_value = ui->txtBlkDiscrY->text().toInt();
    par1->getParameter<GSLibParUInt*>(2)->_value = ui->txtBlkDiscrZ->text().toInt();

    //Generate the parameter file
    //NOTE: due to a buggy input file setting in gammabar, the parameter file must be generated
    //      in the same directory of the executable and the path is simply the file name, without a path.
    QString par_file_path = Application::instance()->generateUniqueFilePathInGSLibDir("par");
    gpar.save( par_file_path );

    //to extract the filename from the path
    QFileInfo par_fileinfo( par_file_path );
    par_file_path = par_fileinfo.fileName();

    //run gammabar program
    //NOTE: do not pass the complete path to gammabar.
    Application::instance()->logInfo("Starting gammabar program...");
    GSLib::instance()->runProgram( "gammabar", par_file_path, true );

    //get gammabar output text
    QString gammabar_out = GSLib::instance()->getLastOutput();

    //parse the gammabar text for the average variogram value
    QRegularExpression re("average\\s*variogram\\s*=\\s*([^\\s]+)", QRegularExpression::MultilineOption | QRegularExpression::CaseInsensitiveOption );
    double gamma_value = 0.0;
    QRegularExpressionMatch match = re.match( gammabar_out );
    if( match.hasMatch() ){
        gamma_value = match.captured(1).toDouble();
    }

    //compute variance loss due to current grid resolution against the selected variogram model.
    double variance_loss = gamma_value / vm->getSill() * 100;

    //display result
    ui->lblVarianceLoss->setText("<html><head/><body><p><span style=\" font-weight:600; color:#0000ff;\">Variance Loss = " +
                                 QString::number( variance_loss, 'f', 1) + "%</span></p></body></html>");

    //delete the parameter file generated in the GSLib directory.
    QFile file( par_fileinfo.absoluteFilePath() );
    file.remove();
}

void CreateGridDialog::createGridAndClose()
{
    bool ok;
    //propose a name based on the point set name.
    QString proposed_name( m_pointSet->getName() );
    proposed_name.append( ".grid" );
    QString new_cg_name = QInputDialog::getText(this, "Name the new grid file",
                                             "New grid file name:", QLineEdit::Normal,
                                             proposed_name, &ok);

    if (ok && !new_cg_name.isEmpty()){
        //read values entered by the user
        m_gridParameters->updateValue( m_par );

        //make a tmp file path
        QString tmp_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");

        //create a new grid object corresponding to a file to be created in the tmp directory
        CartesianGrid* cg = new CartesianGrid( tmp_file_path );

        //set the geometry info entered by the user
        cg->setInfoFromGridParameter( m_par );

        //create the physical GEO-EAS grid file with a binary values with a checkerboard pattern.
        Util::createGEOEAScheckerboardGrid( cg, tmp_file_path );

        //import the newly created grid file as a project item
        Application::instance()->getProject()->importCartesianGrid( cg, new_cg_name );

        //close dialog
        emit this->accept();
    }
}

void CreateGridDialog::calcN()
{
    //read values entered by the user
    m_gridParameters->updateValue( m_par );
    //to make sure all data will be within the grid, checks the grid origin parameters
    //correcting origin coordinates as needed.
    if( m_par->_specs_x->getParameter<GSLibParDouble*>(1)->_value >
        m_pointSet->min( m_pointSet->getXindex()-1 ) )
        m_par->_specs_x->getParameter<GSLibParDouble*>(1)->_value = m_pointSet->min( m_pointSet->getXindex()-1 );
    if( m_par->_specs_y->getParameter<GSLibParDouble*>(1)->_value >
        m_pointSet->min( m_pointSet->getYindex()-1 ) )
        m_par->_specs_y->getParameter<GSLibParDouble*>(1)->_value = m_pointSet->min( m_pointSet->getYindex()-1 );
    if( m_pointSet->is3D() &&
        m_par->_specs_z->getParameter<GSLibParDouble*>(1)->_value >
        m_pointSet->min( m_pointSet->getZindex()-1 ) )
        m_par->_specs_z->getParameter<GSLibParDouble*>(1)->_value = m_pointSet->min( m_pointSet->getZindex()-1 );

    //get the deltas from what the user entered
    double xmin = m_par->_specs_x->getParameter<GSLibParDouble*>(1)->_value;
    double xmax = m_pointSet->max( m_pointSet->getXindex()-1 );
    double ymin = m_par->_specs_y->getParameter<GSLibParDouble*>(1)->_value;
    double ymax = m_pointSet->max( m_pointSet->getYindex()-1 );
    double deltaX =  fabs(xmax - xmin);
    double deltaY =  fabs(ymax - ymin);
    double deltaZ;
    if( m_pointSet->is3D() ){
        double zmin =m_par->_specs_z->getParameter<GSLibParDouble*>(1)->_value;
        double zmax = m_pointSet->max( m_pointSet->getZindex()-1 );
        deltaZ = fabs(zmax - zmin);
    } else {
        deltaZ = 1.0;
    }
    bool isFlat = ( !m_pointSet->is3D() || Util::almostEqual2sComplement( 0.0, deltaZ, 1 ) );
    //compute new cell counts by dividing the deltas by the user-given resolutions
    int nx = 1+ceil( deltaX / m_par->_specs_x->getParameter<GSLibParDouble*>(2)->_value );
    int ny = 1+ceil( deltaY / m_par->_specs_y->getParameter<GSLibParDouble*>(2)->_value );
    int nz = isFlat ? 1 : (1+ceil( deltaZ / m_par->_specs_z->getParameter<GSLibParDouble*>(2)->_value ));
    //set the new cell counts
    m_par->_specs_x->getParameter<GSLibParUInt*>(0)->_value = nx;
    m_par->_specs_y->getParameter<GSLibParUInt*>(0)->_value = ny;
    m_par->_specs_z->getParameter<GSLibParUInt*>(0)->_value = nz;
    //update interface
    m_gridParameters->fillFields( m_par );
}

void CreateGridDialog::preview()
{
    //read values entered by the user
    m_gridParameters->updateValue( m_par );

    //make a tmp file path
    QString tmp_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");

    //create a new grid object corresponding to a file to be created in the tmp directory
    CartesianGrid* cg = new CartesianGrid( tmp_file_path );

    //set the geometry info entered by the user
    cg->setInfoFromGridParameter( m_par );

    //create the physical GEO-EAS grid file with a binary values with a checkerboard pattern.
    Util::createGEOEAScheckerboardGrid( cg, tmp_file_path );

    //calling this again to update the variable collection, now that we have a physical file
    cg->setInfoFromGridParameter( m_par );

    //get the variable with the checkerboard parameter
    Attribute* bin_var = (Attribute*)cg->getChildByIndex( 0 );

    //open the plot dialog
    Util::viewGrid( bin_var, this );

    //TODO: delete the cg object created here in a manner that the dialog doesn't crash.
}

void CreateGridDialog::onVariogramClicked()
{
    //get selected variogram model
    VariogramModel* vm = m_vModelList->getSelectedVModel();

    //compute nugget proportion .
    double nugget_prop = vm->getNugget() / vm->getSill() * 100;

    //display result
    ui->lblNugget->setText("<html><head/><body><p><span style=\" font-weight:600; color:#ff0000;\">Nugget = " +
                           QString::number( nugget_prop, 'f', 1) + "%</span></p></body></html>");


}
