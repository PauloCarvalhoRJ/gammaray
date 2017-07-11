#include "multivariogramdialog.h"
#include "ui_multivariogramdialog.h"

#include "domain/application.h"
#include "domain/attribute.h"
#include "domain/file.h"
#include "domain/cartesiangrid.h"
#include "domain/project.h"
#include "util.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "gslib/gslibparametersdialog.h"
#include "gslib/gslib.h"
#include "dialogs/displayplotdialog.h"

MultiVariogramDialog::MultiVariogramDialog(const std::vector<Attribute *> attributes,
                                           QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MultiVariogramDialog),
    m_attributes( attributes ),
    m_gpf_gam( nullptr ),
    m_gpf_vargplt( nullptr )
{
    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    ui->setupUi(this);

    setWindowTitle("Multiple variogram plot");

    //formats a check list for the user
    QString html = "<html><head/><body><p><span style=\" color:#0000ff;\"><table>";
    html += "<tr><td><b>File:</b></td><td>&nbsp;&nbsp;&nbsp;&nbsp;</td><td><b>Variable:</b></td></tr>";
    std::vector<Attribute *>::iterator it = m_attributes.begin();
    for(; it != m_attributes.end(); ++it){
        Attribute *at = *it;
        File *file = at->getContainingFile();
        html += "<tr><td>" + file->getName() + "</td><td>&nbsp;&nbsp;&nbsp;&nbsp;</td><td>"
                + at->getName() + "</td></tr>";
    }
    html += "</table></span></p></body></html>";

    ui->lblCheckList->setText( html );

}

MultiVariogramDialog::~MultiVariogramDialog()
{
    Application::instance()->logInfo("MultiVariogramDialog destroyed.");
    if( m_gpf_vargplt )
        delete m_gpf_vargplt;
    if( m_gpf_gam )
        delete m_gpf_gam;
    delete ui;
}

void MultiVariogramDialog::onGam()
{
    //Attributes of Cartesian grids with the same cell sizes
    //Multiple variograms accross different files only make sense
    //if they share the same support
    std::vector<Attribute *> validAttributes;

    //for each selected variable
    std::vector<Attribute *>::iterator it = m_attributes.begin();
    for(; it != m_attributes.end(); ++it){
        Attribute *at = *it;
        File *file = at->getContainingFile();
        if( file->getFileType() == "CARTESIANGRID" ){
            validAttributes.push_back( at );
        } else {
            Application::instance()->logError("MultiVariogramDialog::onGam(): files of type " + file->getFileType() +
                                              " are not allowed.  File " + file->getName() + " was ignored.");
        }
    }

    if( validAttributes.empty() ){
        Application::instance()->logError("MultiVariogramDialog::onGam(): no valid attribute remained.  Did nothing.");
        return;
    }

    //=============================parameter building from here==================================

    //first build a gam parameter template for all files/variables
    //if the parameter file object was not constructed
    if( ! m_gpf_gam ){

        //Construct an object composition based on the parameter file template for the gam program.
        m_gpf_gam = new GSLibParameterFile( "gam" );

        //Set default values so we need to change less parameters and let
        //the user change the others as one may see fit.
        m_gpf_gam->setDefaultValues();

        //----------------set the minimum required gam paramaters-----------------------
        //input file
        m_gpf_gam->getParameter<GSLibParFile*>(0)->_path = "AUTOMATICALLY SET";
    }
    //--------------------------------------------------------------------------------

    //show the parameter dialog so the user can adjust other settings before running gam
    GSLibParametersDialog gslibpardiag( m_gpf_gam );
    int result = gslibpardiag.exec();

    //if the user didn't cancel the dialog...
    if( result == QDialog::Accepted ){
        std::vector<QString> expVarFilePaths;

        //set attributes that must vary for each variable...
        //TODO: these can be done in parallel
        it = validAttributes.begin();
        for(; it != validAttributes.end(); ++it){
            //get the Attribute
            Attribute *at = *it;

            //can assume the file is a Cartesian grid.
            CartesianGrid* cg = (CartesianGrid*)at->getContainingFile();
            Application::instance()->logInfo("Processing attribute " + at->getName() + " of file " + cg->getName() + "...");

            //loads data in file.
            cg->loadData();

            //get the variable index(es) in parent data file
            uint var_index = cg->getFieldGEOEASIndex( at->getName() );

            //get the max and min of the variable
            double data_min = cg->min( var_index-1 );
            double data_max = cg->max( var_index-1 );

            //input file
            m_gpf_gam->getParameter<GSLibParFile*>(0)->_path = cg->getPath();

            //variable to compute variogram for
            GSLibParMultiValuedFixed *par1 = m_gpf_gam->getParameter<GSLibParMultiValuedFixed*>(1);
            par1->getParameter<GSLibParUInt*>(0)->_value = 1; //number of variables
            GSLibParMultiValuedVariable *par1_1 = par1->getParameter<GSLibParMultiValuedVariable*>(1);
            par1_1->getParameter<GSLibParUInt*>(0)->_value = var_index;

            //trimming limits
            GSLibParMultiValuedFixed *par2 = m_gpf_gam->getParameter<GSLibParMultiValuedFixed*>(2);
            par2->getParameter<GSLibParDouble*>(0)->_value = data_min;
            par2->getParameter<GSLibParDouble*>(1)->_value = data_max;

            //grid definition parameters (assuming the grids have the same cell size so the
            //variograms plotted together make sense)
            GSLibParGrid* par5 = m_gpf_gam->getParameter<GSLibParGrid*>(5);
            par5->_specs_x->getParameter<GSLibParUInt*>(0)->_value = cg->getNX(); //nx
            par5->_specs_x->getParameter<GSLibParDouble*>(1)->_value = cg->getX0(); //min x
            par5->_specs_x->getParameter<GSLibParDouble*>(2)->_value = cg->getDX(); //cell size x
            par5->_specs_y->getParameter<GSLibParUInt*>(0)->_value = cg->getNY(); //ny
            par5->_specs_y->getParameter<GSLibParDouble*>(1)->_value = cg->getY0(); //min y
            par5->_specs_y->getParameter<GSLibParDouble*>(2)->_value = cg->getDY(); //cell size y
            par5->_specs_z->getParameter<GSLibParUInt*>(0)->_value = cg->getNZ(); //nz
            par5->_specs_z->getParameter<GSLibParDouble*>(1)->_value = cg->getZ0(); //min z
            par5->_specs_z->getParameter<GSLibParDouble*>(2)->_value = cg->getDZ(); //cell size z

            //realization number (assumes only one realization per file)
            // Letting the user change it (one may do some weird cross-simulation analysis...)
            //m_gpf_gam->getParameter<GSLibParUInt*>(4)->_value = 1;

            //set an output file with experimental variogram values
            m_gpf_gam->getParameter<GSLibParFile*>(3)->_path =
                    Application::instance()->getProject()->generateUniqueTmpFilePath("out");
            expVarFilePaths.push_back( m_gpf_gam->getParameter<GSLibParFile*>(3)->_path );

            //Generate the parameter file
            QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
            m_gpf_gam->save( par_file_path );

            //...run gam program
            Application::instance()->logInfo("Starting gam program for variable " +
                                             at->getName() + " in file " + cg->getName() + "...");
            GSLib::instance()->runProgram( "gam", par_file_path );
        }


        onVargplt( expVarFilePaths );
    }
}

void MultiVariogramDialog::onVargplt( std::vector<QString> &expVarFilePaths )
{
    //compute a number of variogram curves to plot depending on
    //the experimental variogram calculation parameters
    uint nCurves = 1; //default is one variogram to plot ( nDirections * nVariograms * nVariables )
    uint nDirections = 1; //default is one direction
    uint nVariograms = 1; //default is one variogram
    uint nVariables = expVarFilePaths.size(); //one experimental variogram file per variable

    //surely we're running gam
    {
        GSLibParMultiValuedFixed *par6 = m_gpf_gam->getParameter<GSLibParMultiValuedFixed*>(6);
        nDirections = par6->getParameter<GSLibParUInt*>(0)->_value;
        nVariograms = m_gpf_gam->getParameter<GSLibParUInt*>(9)->_value;
        nCurves = nVariables * nDirections * nVariograms;
    }

    //make list of captions for the legend text further down
    QStringList legendCaptions;
    for( uint i = 0; i < nDirections; ++i){
        GSLibParRepeat *par7 = m_gpf_gam->getParameter<GSLibParRepeat*>(7); //repeat ndir-times
        GSLibParMultiValuedFixed *par7_0 = par7->getParameter<GSLibParMultiValuedFixed*>(i, 0);
        legendCaptions.append( "STEP X=" + QString::number(par7_0->getParameter<GSLibParInt*>(0)->_value)
                               + " STEP Y=" + QString::number(par7_0->getParameter<GSLibParInt*>(1)->_value)
                               + " STEP Z=" + QString::number(par7_0->getParameter<GSLibParInt*>(2)->_value));
    }

    //make a GLSib parameter object if it wasn't done yet.
    if( ! m_gpf_vargplt ){
        m_gpf_vargplt = new GSLibParameterFile( "vargplt" );

        //Set default values so we need to change less parameters and let
        //the user change the others as one may see fit.
        m_gpf_vargplt->setDefaultValues();

        //make plot/window title
        QString title;
        title.append("Variograms (n-variables/n-files)");

        //--------------------set some parameter values-----------------------

        //postscript file
        m_gpf_vargplt->getParameter<GSLibParFile*>(0)->_path =
                Application::instance()->getProject()->generateUniqueTmpFilePath("ps");

        //suggest display settings for each variogram curve
        GSLibParRepeat *par6 = m_gpf_vargplt->getParameter<GSLibParRepeat*>(6); //repeat nvarios-times
        par6->setCount( nCurves );
        //the experimental curves
        for(uint iVariable = 0; iVariable < nVariables; ++iVariable){
            for(uint iVar = 0; iVar < nVariograms; ++iVar){
                for(uint iDir = 0; iDir < nDirections; ++iDir){
                    int i = iVar + iDir*nVariograms + iVariable*nDirections*nVariograms;
                    par6->getParameter<GSLibParFile*>(i, 0)->_path = expVarFilePaths[iVariable];
                    GSLibParMultiValuedFixed *par6_0_1 = par6->getParameter<GSLibParMultiValuedFixed*>(i, 1);
                    par6_0_1->getParameter<GSLibParUInt*>(0)->_value = iDir+iVar*nDirections + 1;
                    par6_0_1->getParameter<GSLibParUInt*>(1)->_value = iDir; //BUG1234: line style should alternat between directions
                    par6_0_1->getParameter<GSLibParOption*>(2)->_selected_value = 0;
                    par6_0_1->getParameter<GSLibParOption*>(3)->_selected_value = 1;
                    par6_0_1->getParameter<GSLibParColor*>(4)->_color_code = (iVariable % 15) + 1; //cycle through the available colors (except the gray tones)
                }
            }
        }

        //plot title
        m_gpf_vargplt->getParameter<GSLibParString*>(5)->_value = title;
    }

    //suggest (number of directions * number of variograms * number of variables) from the variogram calculation parameters
    m_gpf_vargplt->getParameter<GSLibParUInt*>(1)->_value = nCurves; // nvarios

    //adjust curve count as needed
    GSLibParRepeat *par6 = m_gpf_vargplt->getParameter<GSLibParRepeat*>(6); //repeat nvarios-times
    uint old_count = par6->getCount();
    par6->setCount( nCurves );

    //determine wether the number of curves changed
    bool curve_count_changed = ( old_count != nCurves  );

    //suggest experimental variogram visual parameters if the number of curves changes
    //also updates color legend
    for(uint iVariable = 0; iVariable < nVariables; ++iVariable){
        for(uint iDirVar = 0; iDirVar < nDirections*nVariograms; ++iDirVar){
            int i = iDirVar + iVariable * nDirections*nVariograms;
            GSLibParMultiValuedFixed *par6_0_1 = par6->getParameter<GSLibParMultiValuedFixed*>(i, 1);
            par6_0_1->getParameter<GSLibParUInt*>(0)->_value = iDirVar + 1;
            uint colorCode = 1;
            if( curve_count_changed ){
                colorCode = (iVariable % 15) + 1; //cycle through the available colors (except the gray tones)
                par6_0_1->getParameter<GSLibParColor*>(4)->_color_code = colorCode; //cycle through the available colors (except the gray tones)
            } else {
                colorCode = par6_0_1->getParameter<GSLibParColor*>(4)->_color_code;
            }
            // TODO: add color legend per variable
//            if( legendCaptions.size() == (int)ndirections )
//                legendCaptions[ i % ndirections ] = Util::getGSLibColorName(colorCode) + ": " + legendCaptions[ i % ndirections ];
//            else
//                Application::instance()->logWarn("VariogramAnalysisDialog::onVargplt(): Generation or update of color legend text in variogram plot not available.");
        }
    }

    //inputs are the outputs of gam for each variable
    for(uint iVariable = 0; iVariable < nVariables; ++iVariable){
        for(uint iDirVar = 0; iDirVar < nDirections*nVariograms; ++iDirVar){
            int i = iDirVar + iVariable * nDirections*nVariograms;
            par6->getParameter<GSLibParFile*>(i, 0)->_path = expVarFilePaths[iVariable];
        }
    }

    //save and use a txt file to serve as legend
    if( ! legendCaptions.isEmpty() ){
        QString legendTXTfilePath = Application::instance()->getProject()->generateUniqueTmpFilePath("txt");
        m_gpf_vargplt->getParameter<GSLibParFile*>(7)->_path = legendTXTfilePath;
        Util::saveText( legendTXTfilePath, legendCaptions);
    }

    //----------------------display plot------------------------------------------------------------

    //Generate the parameter file
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    m_gpf_vargplt->save( par_file_path );

    //run vargplt program
    Application::instance()->logInfo("Starting vargplt program...");
    GSLib::instance()->runProgram( "vargplt", par_file_path );

    GSLibParameterFile gpf = *m_gpf_vargplt;

    //display the plot output
    DisplayPlotDialog *dpd = new DisplayPlotDialog(m_gpf_vargplt->getParameter<GSLibParFile*>(0)->_path,
                                                   m_gpf_vargplt->getParameter<GSLibParString*>(5)->_value,
                                                   gpf,
                                                   this);
    dpd->show();

}
