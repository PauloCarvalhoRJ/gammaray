#include "nscoredialog.h"
#include "ui_nscoredialog.h"
#include "domain/attribute.h"
#include "domain/application.h"
#include "domain/datafile.h"
#include "domain/project.h"
#include "domain/pointset.h"
#include "domain/cartesiangrid.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslibparametersdialog.h"
#include "gslib/gslib.h"
#include "displayplotdialog.h"
#include "util.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>
#include <QFileInfo>
#include <cmath>

NScoreDialog::NScoreDialog(Attribute *attribute, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NScoreDialog),
    m_attribute( attribute ),
    m_gpf_nscore( nullptr )
{
    ui->setupUi(this);

    this->setWindowTitle( "N-Score on " + m_attribute->getContainingFile()->getName() );

    QString vars_text("Variable: <b><br>");
    vars_text.append( m_attribute->getName() ).append("<br>");
    vars_text.append( "<b>" );
    ui->lblVariable->setText( vars_text );

    if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI ){
        ui->btnParams->setIcon( QIcon(":icons32/setting32") );
        ui->btnHistogram->setIcon( QIcon(":icons32/histo32") );
        ui->btnSave->setIcon( QIcon(":icons32/save32") );
    }

    adjustSize();
}

NScoreDialog::~NScoreDialog()
{
    Application::instance()->logInfo("Normal score dialog destroyed.");
    delete ui;
}

void NScoreDialog::onParams()
{
    //instantiate GSLib parameter file object
    m_gpf_nscore = new GSLibParameterFile( "nscore" );

    //get data file
    DataFile* input_file = static_cast<DataFile*>(m_attribute->getContainingFile());

    //load its data
    input_file->loadData();

    //input file path
    m_gpf_nscore->getParameter<GSLibParFile*>(0)->_path = input_file->getPath();

    //set variable index and weight
    GSLibParMultiValuedFixed* par1 = m_gpf_nscore->getParameter<GSLibParMultiValuedFixed*>(1);
    par1->getParameter<GSLibParUInt*>(0)->_value = input_file->getFieldGEOEASIndex( m_attribute->getName() );
    par1->getParameter<GSLibParUInt*>(1)->_value = 0;

    //get min and max of variable
    double data_min = input_file->min( m_attribute->getIndexInParent() );
    double data_max = input_file->max( m_attribute->getIndexInParent() );

    //trimming limits
    GSLibParMultiValuedFixed *par2 = m_gpf_nscore->getParameter<GSLibParMultiValuedFixed*>(2);
    par2->getParameter<GSLibParDouble*>(0)->_value = data_min - fabs( data_min/100.0 );
    par2->getParameter<GSLibParDouble*>(1)->_value = data_max + fabs( data_max/100.0 );

    //use reference distribution?
    m_gpf_nscore->getParameter<GSLibParOption*>(3)->_selected_value = 0;

    //file with reference distribution
    m_gpf_nscore->getParameter<GSLibParFile*>(4)->_path = "NO FILE";

    //reference distribution's variable index and weight
    GSLibParMultiValuedFixed* par5 = m_gpf_nscore->getParameter<GSLibParMultiValuedFixed*>(5);
    par5->getParameter<GSLibParUInt*>(0)->_value = 0;
    par5->getParameter<GSLibParUInt*>(1)->_value = 0;

    //output file
    m_gpf_nscore->getParameter<GSLibParFile*>(6)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("out");

    //file with transform table
    m_gpf_nscore->getParameter<GSLibParFile*>(7)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("trn");

    //construct the parameter dialog so the user can adjust settings before running nscoremv
    bool discard_parameters = false;
    {
        GSLibParametersDialog gslibpardiag ( m_gpf_nscore );
        int result = gslibpardiag.exec();
        if( result == QDialog::Accepted ){
            //Generate the parameter file
            QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
            m_gpf_nscore->save( par_file_path );
            //run nscore program
            Application::instance()->logInfo("Starting nscore program...");
            GSLib::instance()->runProgram( "nscore", par_file_path );
        } else {
            discard_parameters = true;
        }
    }
    if( discard_parameters ){
        delete m_gpf_nscore;
        m_gpf_nscore = nullptr;
    }
}

void NScoreDialog::onHistogram()
{
    if( ! m_gpf_nscore ){
        QMessageBox::critical( this, "Error", "You must first compute normal scores at least once.");
        return;
    }

    //get the original data file.
    DataFile* original_data_file = dynamic_cast<DataFile*>(m_attribute->getContainingFile());

    //determine whether it is a point set or grid
    bool isPointSet = ( original_data_file->getFileType() == "POINTSET" );

    //histogram computation is different for each data file type
    if( isPointSet ){
        doHistogramPointSet();
    } else {
        doHistogramGrid();
    }
}

void NScoreDialog::onSave()
{
    if( ! m_gpf_nscore ){
        QMessageBox::critical( this, "Error", "You must first compute normal scores at least once.");
        return;
    }

    //get the original data file before n-score
    DataFile* original_data_file = dynamic_cast<DataFile*>(m_attribute->getContainingFile());

    //determine whether the original data file is a point set or grid
    bool isPointSet = ( original_data_file->getFileType() == "POINTSET" );

    //get the DataFile resulted from n-score
    DataFile* input_data_file;
    if( isPointSet ){
        input_data_file = new PointSet( m_gpf_nscore->getParameter<GSLibParFile*>(6)->_path );
        PointSet *pointset_aspect = (PointSet*)input_data_file;
        pointset_aspect->setInfo( pointset_aspect->getXindex(),
                                  pointset_aspect->getYindex(),
                                  pointset_aspect->getZindex(),
                                  "-999");
    } else {
        input_data_file = new CartesianGrid( m_gpf_nscore->getParameter<GSLibParFile*>(6)->_path );
        ((CartesianGrid*)input_data_file)->setInfoFromOtherCG( (CartesianGrid*)original_data_file );
    }

    //nscore writes the normal variable as the last one in the data file
    ProjectComponent* pc = input_data_file->getChildByIndex( input_data_file->getChildCount()-1 );

    //get the variable name generated by nscore
    QString last_attr_name = pc->getName();

    //presents a dialog so the user can change the default name given by nscores.
    bool ok;
    QString proposed_name(m_attribute->getName());
    proposed_name = proposed_name.append("_ns");
    QString new_var_name = QInputDialog::getText(this, "Rename the normal variable",
                                             "New variable name:", QLineEdit::Normal,
                                             proposed_name, &ok);
    if (ok && !new_var_name.isEmpty()){
        //renames the new variable in the physical file.
        Util::renameGEOEASvariable( input_data_file->getPath(), last_attr_name, new_var_name );
        //get the variable index in the GEO-EAS file
        uint indexGEOEASvariable = original_data_file->getFieldGEOEASIndex( m_attribute->getName() );
        //get the normal variable index in the GEO-EAS file
        uint indexGEOEASweight = input_data_file->getFieldGEOEASIndex( new_var_name );
        //copies the transform table to the project directory
        Util::copyFileToDir( m_gpf_nscore->getParameter<GSLibParFile*>(7)->_path,
                        Application::instance()->getProject()->getPath() );
        //get the file name of the transform table
        QString trn_file_name = QFileInfo( m_gpf_nscore->getParameter<GSLibParFile*>(7)->_path ).fileName();
        //sets the variable-normal variable relationship
        original_data_file->addVariableNScoreVariableRelationship( indexGEOEASvariable, indexGEOEASweight, trn_file_name);
        //replaces the data file's physical file with the generated one.
        original_data_file->replacePhysicalFile( input_data_file->getPath() );
    }

}

void NScoreDialog::doHistogramPointSet()
{
    //get the original data file.
    PointSet* original_data_file = (PointSet*)m_attribute->getContainingFile();

    //parse and get input data file
    //the resulting file of nscore in this case is a point set
    PointSet* input_data_file = new PointSet( m_gpf_nscore->getParameter<GSLibParFile*>(6)->_path );
    input_data_file->setInfo( original_data_file->getXindex(),
                              original_data_file->getYindex(),
                              original_data_file->getZindex(),
                              "-999");

    doHistogramCommon( input_data_file );
}

void NScoreDialog::doHistogramGrid()
{
    //get the original data file.
    CartesianGrid* original_data_file = (CartesianGrid*)m_attribute->getContainingFile();

    //parse and get input data file
    //the resulting file of nscore in this case is a point set
    CartesianGrid* input_data_file = new CartesianGrid( m_gpf_nscore->getParameter<GSLibParFile*>(6)->_path );
    input_data_file->setInfoFromOtherCG( original_data_file );

    doHistogramCommon( input_data_file );
}

void NScoreDialog::doHistogramCommon(DataFile *input_data_file)
{
    //load file data.
    input_data_file->loadData();

    //nscore always puts the new transformed variable as the last column
    uint nscore_variable_index = input_data_file->getLastFieldGEOEASIndex();

    //make plot/window title
    QString title = input_data_file->getName();
    title.append("/");
    title.append(m_attribute->getName());
    title.prepend("N-scores of ");

    //Construct an object composition based on the parameter file template for the hisplt program.
    GSLibParameterFile gpf( "histplt" );

    //Set default values so we need to change less parameters and let
    //the user change the others as one may see fit.
    gpf.setDefaultValues();

    //get the maximum and minimun of selected variable, excluding no-data value
    double data_min = input_data_file->min( nscore_variable_index-1 );
    double data_max = input_data_file->max( nscore_variable_index-1 );

    //----------------set the minimum required histplt paramaters-----------------------
    //input parameters (input file, variable and trimming limits)
    GSLibParInputData* par0;
    par0 = gpf.getParameter<GSLibParInputData*>(0);
    par0->_file_with_data._path = input_data_file->getPath();
    par0->_var_wgt_pairs.first()->_var_index = nscore_variable_index;
    par0->_var_wgt_pairs.first()->_wgt_index = 0;
    par0->_trimming_limits._min = data_min - fabs( data_min/100.0 );
    par0->_trimming_limits._max = data_max + fabs( data_max/100.0 );

    //postscript file
    gpf.getParameter<GSLibParFile*>(1)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath("ps");

    //plot title
    gpf.getParameter<GSLibParString*>(9)->_value = title;

    //reference value for the box plot
    gpf.getParameter<GSLibParDouble*>(11)->_value = input_data_file->mean( nscore_variable_index-1 );
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

    //discard the locally constructed CartesianGrid object
    delete input_data_file;
}
