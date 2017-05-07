#include "declusteringdialog.h"
#include "ui_declusteringdialog.h"
#include "domain/pointset.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "domain/application.h"
#include "gslib/gslibparametersdialog.h"
#include "domain/attribute.h"
#include "domain/project.h"
#include "gslib/gslib.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include <cmath>
#include <QMessageBox>
#include "filecontentsdialog.h"
#include "displayplotdialog.h"
#include <QInputDialog>
#include "util.h"

DeclusteringDialog::DeclusteringDialog(Attribute *attribute, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DeclusteringDialog),
    m_attribute( attribute ),
    m_gpf_declus( nullptr )
{
    ui->setupUi(this);
    ui->lblTitle->setText("variable: <font color=\"red\"><b>" + m_attribute->getName() + "</b></color>" );
    this->setWindowTitle( QString("Declustering ").append( m_attribute->getContainingFile()->getName() ) );

    if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI ){
        ui->btnParameters->setIcon( QIcon(":icons32/setting32") );
        ui->btnHistogram->setIcon( QIcon(":icons32/histo32") );
        ui->btnSave->setIcon( QIcon(":icons32/save32") );
        ui->btnLocmap->setIcon( QIcon(":icons32/locmap32") );
        ui->btnSummary->setIcon( QIcon(":icons32/summary32") );
    }


    adjustSize();
}

DeclusteringDialog::~DeclusteringDialog()
{
    delete ui;
    if( m_gpf_declus )
        delete m_gpf_declus;
    Application::instance()->logInfo("Declustering dialog destroyed.");
}

void DeclusteringDialog::onDeclus()
{
    if( ! m_gpf_declus ){
        //get input data file
        //the parent component of an attribute is a file
        //assumes the file is a Point Set, because declus is not used for grids (regular data)
        PointSet* input_data_file = (PointSet*)m_attribute->getContainingFile();

        //loads data in file, because it's necessary.
        input_data_file->loadData();

        //get the variable index in parent data file
        uint var_index = input_data_file->getFieldGEOEASIndex( m_attribute->getName() );

        //Construct an object composition based on the parameter file template for the declus program.
        m_gpf_declus = new GSLibParameterFile( "declus" );

        //Set default values so we need to change less parameters and let
        //the user change the others as one may see fit.
        m_gpf_declus->setDefaultValues();

        //get the max and min of the selected variable
        double data_min = input_data_file->min( var_index-1 );
        double data_max = input_data_file->max( var_index-1 );

        //----------------set the minimum required locmap paramaters-----------------------

        //file with data
        m_gpf_declus->getParameter<GSLibParFile*>(0)->_path = input_data_file->getPath();

        //X, Y, Z and variable columns
        GSLibParMultiValuedFixed* par1;
        par1 = m_gpf_declus->getParameter<GSLibParMultiValuedFixed*>(1);
        par1->getParameter<GSLibParUInt*>(0)->_value = input_data_file->getXindex();
        par1->getParameter<GSLibParUInt*>(1)->_value = input_data_file->getYindex();
        par1->getParameter<GSLibParUInt*>(2)->_value = input_data_file->getZindex();
        par1->getParameter<GSLibParUInt*>(3)->_value = var_index;

        //trimming limits
        GSLibParMultiValuedFixed* par2;
        par2 = m_gpf_declus->getParameter<GSLibParMultiValuedFixed*>(2);
        par2->getParameter<GSLibParDouble*>(0)->_value = data_min;
        par2->getParameter<GSLibParDouble*>(1)->_value = data_max;

        //file for summary
        m_gpf_declus->getParameter<GSLibParFile*>(3)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("out");

        //output data file with the desclustering weights
        m_gpf_declus->getParameter<GSLibParFile*>(4)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("out");

        //suggest some cell sizes, dividing total width by 20.
        double base_size = fabs(input_data_file->max( input_data_file->getXindex()-1 ) - \
                           input_data_file->min( input_data_file->getXindex()-1 )) / 20.0;

        //number of cell sizes, min size, max size
        GSLibParMultiValuedFixed* par7;
        par7 = m_gpf_declus->getParameter<GSLibParMultiValuedFixed*>(7);
        par7->getParameter<GSLibParUInt*>(0)->_value = 20;
        par7->getParameter<GSLibParDouble*>(1)->_value = base_size / 4.0;
        par7->getParameter<GSLibParDouble*>(2)->_value = base_size;

        //----------------------------------------------------------------------------------
    }
    //construct the parameter dialog so the user can adjust settings before running declus
    GSLibParametersDialog gslibpardiag ( m_gpf_declus );
    int result = gslibpardiag.exec();
    if( result == QDialog::Accepted ){
        //Generate the parameter file
        QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
        m_gpf_declus->save( par_file_path );
        //run declus program
        Application::instance()->logInfo("Starting declus program...");
        GSLib::instance()->runProgram( "declus", par_file_path );
    } else {
        delete m_gpf_declus;
        m_gpf_declus = nullptr;
    }
}

void DeclusteringDialog::onViewSummary()
{
    if( ! m_gpf_declus ){
        QMessageBox::critical( this, "Error", "You must first compute the declustering at least once.");
        return;
    }
    FileContentsDialog fcd( this, m_gpf_declus->getParameter<GSLibParFile*>(3)->_path, "Declustering summary");
    fcd.exec();
}

void DeclusteringDialog::onHistogram()
{
    if( ! m_gpf_declus ){
        QMessageBox::critical( this, "Error", "You must first compute the declustering at least once.");
        return;
    }

    PointSet* original_data_file = (PointSet*)m_attribute->getContainingFile();


    //parse and get input data file
    //the resulting file of declus is a point set
    PointSet* input_data_file = new PointSet( m_gpf_declus->getParameter<GSLibParFile*>(4)->_path );
    input_data_file->setInfo( original_data_file->getXindex(),
                              original_data_file->getYindex(),
                              original_data_file->getZindex(),
                              "-999");

    //load file data.
    input_data_file->loadData();

    //get the variable index
    uint var_index = input_data_file->getFieldGEOEASIndex( m_attribute->getName() );

    //declus always puts the new computed weights as the last column
    uint declus_weight_index = input_data_file->getLastFieldGEOEASIndex();

    //make plot/window title
    QString title = input_data_file->getName();
    title.append("/");
    title.append(m_attribute->getName());
    title.append(" declustered histogram");

    //Construct an object composition based on the parameter file template for the hisplt program.
    GSLibParameterFile gpf( "histplt" );

    //Set default values so we need to change less parameters and let
    //the user change the others as one may see fit.
    gpf.setDefaultValues();

    //get the maximum and minimun of selected variable, excluding no-data value
    double data_min = input_data_file->min( var_index-1 );
    double data_max = input_data_file->max( var_index-1 );

    //----------------set the minimum required histplt paramaters-----------------------
    //input parameters (input file, variable and trimming limits)
    GSLibParInputData* par0;
    par0 = gpf.getParameter<GSLibParInputData*>(0);
    par0->_file_with_data._path = input_data_file->getPath();
    par0->_var_wgt_pairs.first()->_var_index = var_index;
    par0->_var_wgt_pairs.first()->_wgt_index = declus_weight_index;
    par0->_trimming_limits._min = data_min - fabs( data_min/100.0 );
    par0->_trimming_limits._max = data_max + fabs( data_max/100.0 );

    //postscript file
    gpf.getParameter<GSLibParFile*>(1)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("ps");

    //plot title
    gpf.getParameter<GSLibParString*>(9)->_value = title;

    //reference value for the box plot
    gpf.getParameter<GSLibParDouble*>(11)->_value = input_data_file->mean( var_index-1 );
    //----------------------------------------------------------------------------------

    //Generate the parameter file
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    gpf.save( par_file_path );

    //run histplt program
    Application::instance()->logInfo("Starting histplt program...");
    GSLib::instance()->runProgram( "histplt", par_file_path );

    //display the plot output
    DisplayPlotDialog *dpd = new DisplayPlotDialog(gpf.getParameter<GSLibParFile*>(1)->_path, title, gpf, this);
    dpd->show(); //show() makes dialog modalless

    //discard the locally constructed PointSet object
    delete input_data_file;
}

void DeclusteringDialog::onSave()
{
    if( ! m_gpf_declus ){
        QMessageBox::critical( this, "Error", "You must first compute the declustering at least once.");
        return;
    }

    //get the original Point Set file before declustering
    PointSet* original_data_file = (PointSet*)m_attribute->getContainingFile();

    //get the Point Set resulted from declustering
    PointSet* input_data_file = new PointSet( m_gpf_declus->getParameter<GSLibParFile*>(4)->_path );
    input_data_file->setInfo( original_data_file->getXindex(),
                              original_data_file->getYindex(),
                              original_data_file->getZindex(),
                              "-999");

    //declus writes the declustering weights variable as the last one in the Point Set
    ProjectComponent* pc = input_data_file->getChildByIndex( input_data_file->getChildCount()-1 );

    //get the name generated by declus
    QString last_attr_name = pc->getName();

    //presents a dialog so the user can change the default name given by declus.
    bool ok;
    QString proposed_name(m_attribute->getName());
    proposed_name = proposed_name.append("_wgt");
    QString new_var_name = QInputDialog::getText(this, "Rename declustering weight variable",
                                             "New variable name:", QLineEdit::Normal,
                                             proposed_name, &ok);
    if (ok && !new_var_name.isEmpty()){
        //renames the new variable names in the physical file.
        Util::renameGEOEASvariable( input_data_file->getPath(), last_attr_name, new_var_name );
        //get the variable index in the GEO-EAS file
        uint indexGEOEASvariable = original_data_file->getFieldGEOEASIndex( m_attribute->getName() );
        //get the weight index in the GEO-EAS file
        uint indexGEOEASweight = input_data_file->getFieldGEOEASIndex( new_var_name );
        //sets the variable-weight relationship
        original_data_file->addVariableWeightRelationship( indexGEOEASvariable, indexGEOEASweight );
        //replaces the PointSet's physical file with the generated one.
        original_data_file->replacePhysicalFile( input_data_file->getPath() );
    }
}

void DeclusteringDialog::onLocmap()
{

    if( ! m_gpf_declus ){
        QMessageBox::critical( this, "Error", "You must first compute the declustering at least once.");
        return;
    }

    //get input data file
    //assumes the file is a Point Set, since declus works on point sets
    PointSet* original_data_file = (PointSet*)m_attribute->getContainingFile();

    //parse and get input data file
    //the resulting file of declus is a point set
    PointSet* input_data_file = new PointSet( m_gpf_declus->getParameter<GSLibParFile*>(4)->_path );
    input_data_file->setInfo( original_data_file->getXindex(),
                              original_data_file->getYindex(),
                              original_data_file->getZindex(),
                              "-999");

    //loads data in file, because it's necessary.
    input_data_file->loadData();

    //get the variable index in parent data file
    //declus always puts the new computed weights as the last column
    ProjectComponent* pc = input_data_file->getChildByIndex( input_data_file->getChildCount()-1 );
    QString last_attr_name = pc->getName();
    uint var_index = input_data_file->getFieldGEOEASIndex( last_attr_name );

    //make plot/window title
    QString title = input_data_file->getName();
    title.append("/");
    title.append(m_attribute->getName());
    title.append(" declust. weights map");

    //Construct an object composition based on the parameter file template for the locmap program.
    GSLibParameterFile gpf( "locmap" );

    //Set default values so we need to change less parameters and let
    //the user change the others as one may see fit.
    gpf.setDefaultValues();

    //get the max and min of the selected variable
    double data_min = input_data_file->min( var_index-1 );
    double data_max = input_data_file->max( var_index-1 );

    //----------------set the minimum required locmap paramaters-----------------------
    //input file
    gpf.getParameter<GSLibParFile*>(0)->_path = input_data_file->getPath();

    //X, Y and variable
    GSLibParMultiValuedFixed* par1 = gpf.getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = input_data_file->getXindex();
    par1->getParameter<GSLibParUInt*>(1)->_value = input_data_file->getYindex();
    par1->getParameter<GSLibParUInt*>(2)->_value = var_index;

    //trimming limits
    GSLibParMultiValuedFixed* par2 = gpf.getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParDouble*>(0)->_value = data_min - fabs( data_min/100.0 );
    par2->getParameter<GSLibParDouble*>(1)->_value = data_max + fabs( data_max/100.0 );

    //postscript file
    gpf.getParameter<GSLibParFile*>(3)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("ps");

    //X limits
    GSLibParMultiValuedFixed* par4 = gpf.getParameter<GSLibParMultiValuedFixed*>(4);
    par4->getParameter<GSLibParDouble*>(0)->_value = input_data_file->min( input_data_file->getXindex()-1 );
    par4->getParameter<GSLibParDouble*>(1)->_value = input_data_file->max( input_data_file->getXindex()-1 );

    //Y limits
    GSLibParMultiValuedFixed* par5 = gpf.getParameter<GSLibParMultiValuedFixed*>(5);
    par5->getParameter<GSLibParDouble*>(0)->_value = input_data_file->min( input_data_file->getYindex()-1 );
    par5->getParameter<GSLibParDouble*>(1)->_value = input_data_file->max( input_data_file->getYindex()-1 );

    //color scale details
    GSLibParMultiValuedFixed* par10 = gpf.getParameter<GSLibParMultiValuedFixed*>(10);
    par10->getParameter<GSLibParDouble*>(0)->_value = data_min;
    par10->getParameter<GSLibParDouble*>(1)->_value = data_max;
    par10->getParameter<GSLibParDouble*>(2)->_value = (data_max-data_min)/10.0; //color scale ticks in 10 steps

    //plot title
    gpf.getParameter<GSLibParString*>(12)->_value = title;
    //----------------------------------------------------------------------------------

    //Generate the parameter file
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    gpf.save( par_file_path );

    //run locmap program
    Application::instance()->logInfo("Starting locmap program...");
    GSLib::instance()->runProgram( "locmap", par_file_path );

    //display the plot output
    DisplayPlotDialog *dpd = new DisplayPlotDialog(gpf.getParameter<GSLibParFile*>(3)->_path, title, gpf, this);
    dpd->show(); //show() makes dialog modalless
}
