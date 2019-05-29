#include "mcrfsimdialog.h"
#include "ui_mcrfsimdialog.h"

#include <QDragEnterEvent>
#include <QMimeData>

#include "domain/application.h"
#include "domain/projectcomponent.h"
#include "domain/attribute.h"
#include "domain/project.h"
#include "domain/categorydefinition.h"
#include "domain/datafile.h"
#include "widgets/fileselectorwidget.h"
#include "widgets/variableselector.h"
#include "widgets/cartesiangridselector.h"

MCRFSimDialog::MCRFSimDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MCRFSimDialog)
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    setWindowTitle( "Markov Chains Random Field Simulation" );

    m_primFileSelector = new FileSelectorWidget( FileSelectorType::PointAndSegmentSets );
    ui->frmCmbPrimFile->layout()->addWidget( m_primFileSelector );

    m_primVarSelector = new VariableSelector( false, VariableSelectorType::CATEGORICAL );
    ui->frmCmbPrimVar->layout()->addWidget( m_primVarSelector );

    connect( m_primFileSelector, SIGNAL(dataFileSelected(DataFile*)),
             m_primVarSelector,  SLOT(onListVariables(DataFile*)) );

    //calling this slot causes the variable comboboxes to update, so they show up populated
    //otherwise the user is required to choose another file and then back to the first file
    //if the desired sample file happens to be the first one in the list.
    m_primFileSelector->onSelection( 0 );

    //The list with existing cartesian grids in the project for the secondary data.
    m_simGridSelector = new CartesianGridSelector( );
    ui->frmSimGrid->layout()->addWidget( m_simGridSelector );

    m_verticalTransiogramSelector = new FileSelectorWidget( FileSelectorType::VerticalTransiogramModels );
    ui->frmTransiogramSelector->layout()->addWidget( m_verticalTransiogramSelector );

}

MCRFSimDialog::~MCRFSimDialog()
{
    delete ui;
}

