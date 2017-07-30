#include "imagejockeydialog.h"
#include "ui_imagejockeydialog.h"
#include "widgets/cartesiangridselector.h"
#include "widgets/variableselector.h"
#include "domain/application.h"
#include "imagejockeygridplot.h"

ImageJockeyDialog::ImageJockeyDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImageJockeyDialog)
{
    ui->setupUi(this);

    setWindowTitle( "Image Jockey" );

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    m_gridPlot = new ImageJockeyGridPlot();
    ui->frmGridPlot->layout()->addWidget( m_gridPlot );

    //the combo box to choose a Cartesian grid containing a Fourier image
    m_cgSelector = new CartesianGridSelector( true );
    ui->frmCmbGridPlaceholder->layout()->addWidget( m_cgSelector );

    //the combo box to choose the variable with the real part
    m_atSelector = new VariableSelector();
    ui->frmCmbAttributePlaceholder->layout()->addWidget( m_atSelector );
    connect( m_cgSelector, SIGNAL(cartesianGridSelected(DataFile*)),
             m_atSelector, SLOT(onListVariables(DataFile*)) );
    connect( m_atSelector, SIGNAL(variableSelected(Attribute*)),
             this, SLOT(onUpdateGridPlot()));

    //calling this slot causes the variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_cgSelector->onSelection( 0 );
}

ImageJockeyDialog::~ImageJockeyDialog()
{
    delete ui;
    Application::instance()->logInfo("ImageJockeyDialog destroyed.");
}

void ImageJockeyDialog::onUpdateGridPlot()
{

}
