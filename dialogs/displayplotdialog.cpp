#include "displayplotdialog.h"
#include "ui_displayplotdialog.h"
#include "widgets/pswidget.h"
#include "gslib/gslibparametersdialog.h"
#include "domain/application.h"
#include "domain/project.h"
#include "gslib/gslib.h"
#include "domain/plot.h"
#include <QInputDialog>

DisplayPlotDialog::DisplayPlotDialog(const QString path_to_postscript, const QString diag_title, GSLibParameterFile gpf, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DisplayPlotDialog),
    _gpf( gpf ),
    _ps_file_path( path_to_postscript )
{

    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    //set dialog title
    this->setWindowTitle( diag_title );

    //add the PostScript widget (not originally present in .UI file)
    QVBoxLayout *vbl = new QVBoxLayout();
    vbl->setObjectName("vertical_layout");
    vbl->setSpacing(0);
    vbl->setMargin(0);
    ui->widgetPlace->setLayout( vbl );
    _psw = new PSWidget();
    vbl->addWidget( _psw );

    connect(_psw, SIGNAL(userWantsToChangeParameters()), this, SLOT(onChangeParameters()));
    connect(_psw, SIGNAL(userWantsToSavePlot()), this, SLOT(onSavePlot()));

    //render the PostScript plot
    _psw->displayPS( _ps_file_path );

    adjustSize();
}

DisplayPlotDialog::~DisplayPlotDialog()
{
    delete ui;
}

void DisplayPlotDialog::onChangeParameters()
{
    if( _gpf.isEmpty() )
        return;
    GSLibParametersDialog gsd(&(this->_gpf), this);
    gsd.exec();
    if( gsd.result() == QDialog::Accepted ){
        //Generate the parameter file
        QString par_file_path = Application::instance()->getProject()->generateUniqueTmpFilePath("par");
        this->_gpf.save( par_file_path );
        //run GSLib program
        Application::instance()->logInfo( QString("Starting ").append( this->_gpf.getProgramName() ).append(" program...") );
        GSLib::instance()->runProgram( this->_gpf.getProgramName(), par_file_path );
        //render the PostScript plot
        //TODO: there is no guarantee this _ps_file_path is the same in the parameters file object (_gpf)
        _psw->displayPS( _ps_file_path );
    }
}

void DisplayPlotDialog::onSavePlot()
{
    bool ok;
    QString proposed_name = "Some";
    if( ! _gpf.isEmpty() )
        proposed_name = _gpf.getProgramName() ;
    proposed_name = proposed_name.append("_plot");
    QString new_plot_name = QInputDialog::getText(this, "Name the new plot file",
                                             "New plot file name:", QLineEdit::Normal,
                                             proposed_name, &ok);
    if (ok && !new_plot_name.isEmpty()){
        Application::instance()->getProject()->importPlot( _ps_file_path, new_plot_name.append(".ps") );
    }
}
