#include "distributionmodelingdialog.h"
#include "ui_distributionmodelingdialog.h"

#include "domain/attribute.h"
#include "domain/project.h"
#include "domain/file.h"
#include "gslib/gslibparameterfiles/gslibparameterfile.h"
#include "domain/application.h"
#include "gslib/gslibparametersdialog.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslib.h"
#include "displayplotdialog.h"
#include "distributioncolumnrolesdialog.h"
#include "util.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QLineEdit>

DistributionModelingDialog::DistributionModelingDialog(Attribute *at, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DistributionModelingDialog),
    m_attribute( at ),
    m_gpf_histsmth( nullptr )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle("Model a distribution for " + at->getContainingFile()->getName() + ":" + at->getName());

    if( Util::getDisplayResolutionClass() == DisplayResolution::HIGH_DPI ){
        ui->btnParameters->setIcon( QIcon(":icons32/setting32") );
        ui->btnPlot->setIcon( QIcon(":icons32/plot32") );
        ui->btnSave->setIcon( QIcon(":icons32/save32") );
    }

    adjustSize();
}

DistributionModelingDialog::~DistributionModelingDialog()
{
    if( m_gpf_histsmth )
        delete m_gpf_histsmth;
    delete ui;
    Application::instance()->logInfo( "DistributionModelingDialog destroyed." );
}

void DistributionModelingDialog::onParameters()
{
    if( ! m_gpf_histsmth ){
        m_gpf_histsmth = new GSLibParameterFile("histsmth");
        m_gpf_histsmth->setDefaultValues();

        //make a descriptive title
        QString title = "Distr. model for " + m_attribute->getContainingFile()->getName() + ":" + m_attribute->getName();

        //input data parameters
        GSLibParInputData* par0 = m_gpf_histsmth->getParameter<GSLibParInputData*>(0);
        par0->set( m_attribute );

        //title for the plot
        m_gpf_histsmth->getParameter<GSLibParString*>(1)->_value = title;

        //path for the PostScript plot file
        m_gpf_histsmth->getParameter<GSLibParFile*>(2)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "ps" );

        //path for the distribution model file
        m_gpf_histsmth->getParameter<GSLibParFile*>(4)->_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "dst" );

        //the value limits for the distribution model
        GSLibParMultiValuedFixed *par5 = m_gpf_histsmth->getParameter<GSLibParMultiValuedFixed*>(5);
        par5->getParameter<GSLibParDouble*>(1)->_value = par0->getLowerTrimmingLimit();
        par5->getParameter<GSLibParDouble*>(2)->_value = par0->getUpperTrimmingLimit();
    }

    //show the histsmth parameters
    GSLibParametersDialog gsd( m_gpf_histsmth, this );
    int result = gsd.exec();

    //if user didn't cancel the dialog
    if( result == QDialog::Accepted ){
        //Generate the parameter file
        QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath( "par" );
        m_gpf_histsmth->save( par_file_path );

        //to be notified when histsmth completes.
        connect( GSLib::instance(), SIGNAL(programFinished()), this, SLOT(onHistsmthCompletion()) );

        //run histsmth program
        Application::instance()->logInfo("Starting histsmth program...");
        GSLib::instance()->runProgramAsync( "histsmth", par_file_path );
    }
}

void DistributionModelingDialog::onHistsmthCompletion()
{
    //frees all signal connections to the GSLib singleton.
    GSLib::instance()->disconnect();

    //display the plot output
    DisplayPlotDialog *dpd = new DisplayPlotDialog(m_gpf_histsmth->getParameter<GSLibParFile*>(2)->_path,
                                                   m_gpf_histsmth->getParameter<GSLibParString*>(1)->_value,
                                                   *m_gpf_histsmth,
                                                   this);
    //show dialog non-modally
    dpd->show();
}

void DistributionModelingDialog::onPlot()
{
    if( m_gpf_histsmth )
        onHistsmthCompletion();
    else
        QMessageBox::critical(this, "Error", QString("You must run histsmth at least once."));
}

void DistributionModelingDialog::onSave()
{
    if( !m_gpf_histsmth ){
        QMessageBox::critical( this, "Error", QString("You must run histsmth at least once."));
        return;
    }

    Util::importUnivariateDistribution( m_attribute, m_gpf_histsmth->getParameter<GSLibParFile*>(4)->_path, this );
}
