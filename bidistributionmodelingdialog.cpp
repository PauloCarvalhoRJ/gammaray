#include "bidistributionmodelingdialog.h"
#include "ui_bidistributionmodelingdialog.h"
#include "displayplotdialog.h"
#include "distributioncolumnrolesdialog.h"
#include "util.h"
#include "domain/attribute.h"
#include "domain/file.h"
#include "domain/application.h"
#include "domain/univariatedistribution.h"
#include "domain/datafile.h"
#include "domain/project.h"
#include "widgets/univariatedistributionselector.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslibparametersdialog.h"
#include "gslib/gslib.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <cmath>

BidistributionModelingDialog::BidistributionModelingDialog(Attribute *atX, Attribute *atY, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BidistributionModelingDialog),
    m_atX( atX ),
    m_atY( atY ),
    m_cmbXDist( new UnivariateDistributionSelector() ),
    m_cmbYDist( new UnivariateDistributionSelector() ),
    m_gpf_scatsmth( nullptr ),
    m_gpf_bivplt( nullptr )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    //it is assumed that both variables belong to the same file
    this->setWindowTitle("Model a bidistribution for " + m_atX->getContainingFile()->getName() );

    //set labels to show variable names.
    ui->lblDistrVar1->setText("1) Select distribution for " + m_atX->getName() + ":");
    ui->lblDistrVar2->setText("2) Select distribution for " + m_atY->getName() + ":");

    //add the comboboxes to select the univariate
    //smooth distributions for the variables
    ui->widDistrVar1->layout()->addWidget( m_cmbXDist );
    ui->widDistrVar2->layout()->addWidget( m_cmbYDist );

    adjustSize();
}

BidistributionModelingDialog::~BidistributionModelingDialog()
{
    delete ui;
    if( m_gpf_scatsmth )
        delete m_gpf_scatsmth;
    Application::instance()->logInfo("BidistributionModelingDialog destroyed.");
}

void BidistributionModelingDialog::onParameters()
{
    //get the selected univariate distribution for the X variable
    UnivariateDistribution* Xdistr = m_cmbXDist->getSelectedDistribution();
    if( ! Xdistr ){
        QMessageBox::critical( this, "Error", QString("A univariate distribution for the X variable is required."));
        return;
    }

    //get the selected univariate distribution for the Y variable
    UnivariateDistribution* Ydistr = m_cmbYDist->getSelectedDistribution();
    if( ! Ydistr ){
        QMessageBox::critical( this, "Error", QString("A univariate distribution for the Y variable is required."));
        return;
    }

    if( ! m_gpf_scatsmth ){
        m_gpf_scatsmth = new GSLibParameterFile("scatsmth");
        m_gpf_scatsmth->setDefaultValues();

        //make a descriptive title
        QString title = "Bidistr. model for " + m_atX->getContainingFile()->getName() + ": " + m_atX->getName() + " X " + m_atY->getName();

        //assumes that both variables are from the same data file
        DataFile* data_file = (DataFile*)m_atX->getContainingFile();

        //load the data
        data_file->loadData();

        //get the GEO-EAS index of both variables
        uint varX_index = data_file->getFieldGEOEASIndex( m_atX->getName() );
        uint varY_index = data_file->getFieldGEOEASIndex( m_atY->getName() );

        //get the data max and min for both variables
        double dataX_min = data_file->min( varX_index-1 );
        double dataX_max = data_file->max( varX_index-1 );
        double dataY_min = data_file->min( varY_index-1 );
        double dataY_max = data_file->max( varY_index-1 );

        //add some tolerance to max/min
        dataX_min -= fabs( dataX_min/100.0 );
        dataX_max += fabs( dataX_max/100.0 );
        dataY_min -= fabs( dataY_min/100.0 );
        dataY_max += fabs( dataY_max/100.0 );

        //set input data file
        m_gpf_scatsmth->getParameter<GSLibParFile*>(0)->_path = data_file->getPath();

        //set variable indexes and weight
        GSLibParMultiValuedFixed *par1 = m_gpf_scatsmth->getParameter<GSLibParMultiValuedFixed*>(1);
        par1->getParameter<GSLibParUInt*>(0)->_value = varX_index;
        par1->getParameter<GSLibParUInt*>(1)->_value = varY_index;
        par1->getParameter<GSLibParUInt*>(2)->_value = 0;

        //set the smooth distribution for X variable
        m_gpf_scatsmth->getParameter<GSLibParFile*>(2)->_path = Xdistr->getPath();

        //set the column indexes of the X smooth distribution
        GSLibParMultiValuedFixed *par3 = m_gpf_scatsmth->getParameter<GSLibParMultiValuedFixed*>(3);
        par3->getParameter<GSLibParUInt*>(0)->_value = Xdistr->getTheColumnWithValueRole();
        par3->getParameter<GSLibParUInt*>(1)->_value = Xdistr->getTheColumnWithProbabilityRole();

        //set the smooth distribution for Y variable
        m_gpf_scatsmth->getParameter<GSLibParFile*>(4)->_path = Ydistr->getPath();

        //set the column indexes of the Y smooth distribution
        GSLibParMultiValuedFixed *par5 = m_gpf_scatsmth->getParameter<GSLibParMultiValuedFixed*>(5);
        par5->getParameter<GSLibParUInt*>(0)->_value = Ydistr->getTheColumnWithValueRole();
        par5->getParameter<GSLibParUInt*>(1)->_value = Ydistr->getTheColumnWithProbabilityRole();

        //set the log scaling options
        GSLibParMultiValuedFixed *par6 = m_gpf_scatsmth->getParameter<GSLibParMultiValuedFixed*>(6);
        par6->getParameter<GSLibParOption*>(0)->_selected_value = 0;
        par6->getParameter<GSLibParOption*>(1)->_selected_value = 0;

        //set the output debug file
        m_gpf_scatsmth->getParameter<GSLibParFile*>(7)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "scatsmth.debug" );

        //set the new smooth distribution for the X variable
        m_gpf_scatsmth->getParameter<GSLibParFile*>(8)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "scatsmth.Xdist" );

        //set the new smooth distribution for the Y variable
        m_gpf_scatsmth->getParameter<GSLibParFile*>(9)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "scatsmth.Ydist" );

        //set the output smooth bidistribution file
        m_gpf_scatsmth->getParameter<GSLibParFile*>(10)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "bidist" );

        //set the vertex count for the envelope (trimming limits in 2D)
        m_gpf_scatsmth->getParameter<GSLibParUInt*>(17)->_value = 4;

        //configure the envelope poly as a bounding rectangle defined by the data value max and min values
        //TODO: probably it is necessary to compute the log of limits if the respective distributions are in log scale
        GSLibParRepeat *par18 = m_gpf_scatsmth->getParameter<GSLibParRepeat*>(18); //repeat nenvelope-times
        par18->setCount( m_gpf_scatsmth->getParameter<GSLibParUInt*>(17)->_value );
        GSLibParMultiValuedFixed *par18_0 = par18->getParameter<GSLibParMultiValuedFixed*>(0, 0);
        par18_0->getParameter<GSLibParDouble*>(0)->_value = dataX_min;
        par18_0->getParameter<GSLibParDouble*>(1)->_value = dataY_min;
        GSLibParMultiValuedFixed *par18_1 = par18->getParameter<GSLibParMultiValuedFixed*>(1, 0);
        par18_1->getParameter<GSLibParDouble*>(0)->_value = dataX_min;
        par18_1->getParameter<GSLibParDouble*>(1)->_value = dataY_max;
        GSLibParMultiValuedFixed *par18_2 = par18->getParameter<GSLibParMultiValuedFixed*>(2, 0);
        par18_2->getParameter<GSLibParDouble*>(0)->_value = dataX_max;
        par18_2->getParameter<GSLibParDouble*>(1)->_value = dataY_max;
        GSLibParMultiValuedFixed *par18_3 = par18->getParameter<GSLibParMultiValuedFixed*>(3, 0);
        par18_3->getParameter<GSLibParDouble*>(0)->_value = dataX_max;
        par18_3->getParameter<GSLibParDouble*>(1)->_value = dataY_min;
    }

    //show the scatsmth parameters
    GSLibParametersDialog gsd( m_gpf_scatsmth, this );
    int result = gsd.exec();

    //if user didn't cancel the dialog
    if( result == QDialog::Accepted ){
        //Generate the parameter file
        QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "par" );
        m_gpf_scatsmth->save( par_file_path );

        //to be notified when scatsmth completes.
        connect( GSLib::instance(), SIGNAL(programFinished()), this, SLOT(onScatsmthCompletion()) );

        //run scatsmth program
        Application::instance()->logInfo("Starting scatsmth program...");
        GSLib::instance()->runProgramAsync( "scatsmth", par_file_path );
    }
}

void BidistributionModelingDialog::onScatsmthCompletion()
{
    //frees all signal connections to the GSLib singleton.
    GSLib::instance()->disconnect();

    if( ! m_gpf_bivplt ){
        m_gpf_bivplt = new GSLibParameterFile("bivplt");
        m_gpf_bivplt->setDefaultValues();

        //set the input data file
        m_gpf_bivplt->getParameter<GSLibParFile*>(0)->_path = m_gpf_scatsmth->getParameter<GSLibParFile*>(0)->_path;

        //set the variable columns
        GSLibParMultiValuedFixed *scatsmth_par1 = m_gpf_scatsmth->getParameter<GSLibParMultiValuedFixed*>(1);
        GSLibParMultiValuedFixed *par1 = m_gpf_bivplt->getParameter<GSLibParMultiValuedFixed*>(1);
        par1->getParameter<GSLibParUInt*>(0)->_value = scatsmth_par1->getParameter<GSLibParUInt*>(0)->_value;
        par1->getParameter<GSLibParUInt*>(1)->_value = scatsmth_par1->getParameter<GSLibParUInt*>(1)->_value;
        par1->getParameter<GSLibParUInt*>(2)->_value = scatsmth_par1->getParameter<GSLibParUInt*>(2)->_value;

        //set the log scaling options
        GSLibParMultiValuedFixed *scatsmth_par6 = m_gpf_scatsmth->getParameter<GSLibParMultiValuedFixed*>(6);
        GSLibParMultiValuedFixed *par2 = m_gpf_bivplt->getParameter<GSLibParMultiValuedFixed*>(2);
        par2->getParameter<GSLibParOption*>(0)->_selected_value = scatsmth_par6->getParameter<GSLibParOption*>(0)->_selected_value;
        par2->getParameter<GSLibParOption*>(1)->_selected_value = scatsmth_par6->getParameter<GSLibParOption*>(1)->_selected_value;

        //set the smooth distribution file for the X variable
        m_gpf_bivplt->getParameter<GSLibParFile*>(4)->_path = m_gpf_scatsmth->getParameter<GSLibParFile*>(2)->_path;

        //set the column indexes of the smooth distribution of the X variable
        GSLibParMultiValuedFixed *scatsmth_par3 = m_gpf_scatsmth->getParameter<GSLibParMultiValuedFixed*>(3);
        GSLibParMultiValuedFixed *par5 = m_gpf_bivplt->getParameter<GSLibParMultiValuedFixed*>(5);
        par5->getParameter<GSLibParUInt*>(0)->_value = scatsmth_par3->getParameter<GSLibParUInt*>(0)->_value;
        par5->getParameter<GSLibParUInt*>(1)->_value = scatsmth_par3->getParameter<GSLibParUInt*>(1)->_value;

        //set the smooth distribution file for the Y variable
        m_gpf_bivplt->getParameter<GSLibParFile*>(6)->_path = m_gpf_scatsmth->getParameter<GSLibParFile*>(4)->_path;

        //set the column indexes of the smooth distribution of the Y variable
        GSLibParMultiValuedFixed *scatsmth_par5 = m_gpf_scatsmth->getParameter<GSLibParMultiValuedFixed*>(5);
        GSLibParMultiValuedFixed *par7 = m_gpf_bivplt->getParameter<GSLibParMultiValuedFixed*>(7);
        par7->getParameter<GSLibParUInt*>(0)->_value = scatsmth_par5->getParameter<GSLibParUInt*>(0)->_value;
        par7->getParameter<GSLibParUInt*>(1)->_value = scatsmth_par5->getParameter<GSLibParUInt*>(1)->_value;

        //set the columns of the bidistribution file (normally 1, 2 and 3)
        GSLibParMultiValuedFixed *par9 = m_gpf_bivplt->getParameter<GSLibParMultiValuedFixed*>(9);
        par9->getParameter<GSLibParUInt*>(0)->_value = 1;
        par9->getParameter<GSLibParUInt*>(1)->_value = 2;
        par9->getParameter<GSLibParUInt*>(2)->_value = 3;

        //set the output PS plot file
        m_gpf_bivplt->getParameter<GSLibParFile*>(10)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "ps" );

        //set the plot title
        QString title = m_atX->getName();
        title.append( " X ");
        title.append( m_atY->getName() );
        title.append( " bidistribution" );
        m_gpf_bivplt->getParameter<GSLibParString*>(14)->_value = title;
    }

    //set the bidistribution file (may change between several scatsmth runs
    m_gpf_bivplt->getParameter<GSLibParFile*>(8)->_path = m_gpf_scatsmth->getParameter<GSLibParFile*>(10)->_path;

    //Generate the parameter file
    QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
    m_gpf_bivplt->save( par_file_path );

    //run bivplt program
    Application::instance()->logInfo("Starting bivplt program...");
    GSLib::instance()->runProgram( "bivplt", par_file_path );

    //display the plot output
    DisplayPlotDialog *dpd = new DisplayPlotDialog(m_gpf_bivplt->getParameter<GSLibParFile*>(10)->_path,
                                                   m_gpf_bivplt->getParameter<GSLibParString*>(14)->_value,
                                                   *m_gpf_bivplt,
                                                   this);
    //show dialog non-modally
    dpd->show();
}

void BidistributionModelingDialog::onPlot()
{
    if( m_gpf_scatsmth )
        onScatsmthCompletion();
    else
        QMessageBox::critical(this, "Error", QString("You must run scatsmth at least once."));
}

void BidistributionModelingDialog::onSave()
{
    if( !m_gpf_scatsmth ){
        QMessageBox::critical( this, "Error", QString("You must run scatsmth at least once."));
        return;
    }

    //propose a name for the file
    QString proposed_name( m_atX->getName() );
    proposed_name.append( "_X_" );
    proposed_name.append( m_atY->getName() );
    proposed_name.append("_SMOOTH_BIDISTR");

    //open file rename dialog
    bool ok;
    QString new_dist_model_name = QInputDialog::getText(this, "Name the new file",
                                             "New bivariate distribution model file name:", QLineEdit::Normal,
                                             proposed_name, &ok);

    if (ok && !new_dist_model_name.isEmpty()){

        QString tmpPathOfDistr = m_gpf_bivplt->getParameter<GSLibParFile*>(8)->_path;

        //asks the user to set the roles for each of the distribution file columns.
        DistributionColumnRolesDialog dcrd( tmpPathOfDistr, this );
        dcrd.adjustSize();
        int result = dcrd.exec(); //show modally
        if( result == QDialog::Accepted )
            Application::instance()->getProject()->importBivariateDistribution( tmpPathOfDistr,
                                                                                new_dist_model_name.append(".bidst"),
                                                                                dcrd.getRoles() );
        else
            Application::instance()->getProject()->importBivariateDistribution( tmpPathOfDistr,
                                                                                new_dist_model_name.append(".bidst"),
                                                                                QMap<uint, Roles::DistributionColumnRole>() ); //empty list
    }
}

void BidistributionModelingDialog::onSaveXDistr()
{
    if( !m_gpf_scatsmth ){
        QMessageBox::critical( this, "Error", QString("You must run scatsmth at least once."));
        return;
    }

    Util::importUnivariateDistribution( m_atX, m_gpf_scatsmth->getParameter<GSLibParFile*>(8)->_path, this );
}

void BidistributionModelingDialog::onSaveYDistr()
{
    if( !m_gpf_scatsmth ){
        QMessageBox::critical( this, "Error", QString("You must run scatsmth at least once."));
        return;
    }

    Util::importUnivariateDistribution( m_atY, m_gpf_scatsmth->getParameter<GSLibParFile*>(9)->_path, this );
}
