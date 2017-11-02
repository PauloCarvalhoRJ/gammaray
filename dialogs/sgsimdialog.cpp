#include "sgsimdialog.h"
#include "ui_sgsimdialog.h"
#include "domain/application.h"

#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslibparams/widgets/widgetgslibpargrid.h"
#include "widgets/cartesiangridselector.h"
#include "widgets/pointsetselector.h"
#include "widgets/variableselector.h"
#include "widgets/univariatedistributionselector.h"
#include "widgets/variogrammodelselector.h"
#include "widgets/distributionfieldselector.h"

SGSIMDialog::SGSIMDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SGSIMDialog)
{
    ui->setupUi(this);

    this->setWindowTitle("Sequential Gaussian Simulation - SGSIM");

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    //The grid parameters widget (reusing the one orginially made for the GSLib parameter dialog).
    m_par = new GSLibParGrid("", "", "");
    m_par->_specs_x->getParameter<GSLibParUInt*>(0)->_value = 100; //nx
    m_par->_specs_x->getParameter<GSLibParDouble*>(1)->_value = 0; //min x
    m_par->_specs_x->getParameter<GSLibParDouble*>(2)->_value = 1; //cell size x
    m_par->_specs_y->getParameter<GSLibParUInt*>(0)->_value = 100; //ny
    m_par->_specs_y->getParameter<GSLibParDouble*>(1)->_value = 0; //min y
    m_par->_specs_y->getParameter<GSLibParDouble*>(2)->_value = 1; //cell size y
    m_par->_specs_z->getParameter<GSLibParUInt*>(0)->_value = 1; //nz
    m_par->_specs_z->getParameter<GSLibParDouble*>(1)->_value = 0; //min z
    m_par->_specs_z->getParameter<GSLibParDouble*>(2)->_value = 1; //cell size z
    m_gridParameters = new WidgetGSLibParGrid();
    ui->frmGridGeomPlaceholder->layout()->addWidget( m_gridParameters );
    m_gridParameters->fillFields( m_par );

    //Point set widgets
    m_primVarPSetSelector = new PointSetSelector();
    ui->frmPointSetPlaceholder->layout()->addWidget( m_primVarPSetSelector );
    m_primVarSelector = new VariableSelector();
    ui->frmPrimVarPlaceholder->layout()->addWidget( m_primVarSelector );
    m_primVarWgtSelector = new VariableSelector( true );
    ui->frmPrimVarWgtPlaceholder->layout()->addWidget( m_primVarWgtSelector );
    m_primVarSecVarSelector = new VariableSelector( true );
    ui->frmSecVarPlaceholder->layout()->addWidget( m_primVarSecVarSelector );
    connect( m_primVarPSetSelector, SIGNAL(pointSetSelected(DataFile*)),
             m_primVarSelector, SLOT(onListVariables(DataFile*)) );
    connect( m_primVarPSetSelector, SIGNAL(pointSetSelected(DataFile*)),
             m_primVarWgtSelector, SLOT(onListVariables(DataFile*)) );
    connect( m_primVarPSetSelector, SIGNAL(pointSetSelected(DataFile*)),
             m_primVarSecVarSelector, SLOT(onListVariables(DataFile*)) );
    m_primVarPSetSelector->onSelection( 0 );

    //Data transform widgets
    m_refDistFileSelector = new UnivariateDistributionSelector( true );
    ui->frmRefDistFilePlaceholder->layout()->addWidget( m_refDistFileSelector );
    m_refDistValuesSelector = new DistributionFieldSelector();
    ui->frmDistValuesPlaceholder->layout()->addWidget( m_refDistValuesSelector );
    m_refDistFreqSelector = new DistributionFieldSelector();
    ui->frmDistFreqPlaceholder->layout()->addWidget( m_refDistFreqSelector );
    connect( m_refDistFileSelector, SIGNAL(distributionSelected(Distribution*)),
             m_refDistValuesSelector, SLOT(onListFields(Distribution*)) );
    connect( m_refDistFileSelector, SIGNAL(distributionSelected(Distribution*)),
             m_refDistFreqSelector, SLOT(onListFields(Distribution*)) );

    //Grid geometry widgets
    m_gridCopySpecsSelector = new CartesianGridSelector( true );
    ui->frmCopyGridSpecsPlaceholder->layout()->addWidget( m_gridCopySpecsSelector );

    //Secondary data widgets
    m_secVarGridSelector = new CartesianGridSelector( true );
    ui->frmSecDataGridPlaceholder->layout()->addWidget( m_secVarGridSelector );
    m_secVarVariableSelector = new VariableSelector( true );
    ui->frmSecVarSelectorPlaceholder->layout()->addWidget( m_secVarVariableSelector );

    //Variogram model widgets
    m_vModelSelector = new VariogramModelSelector();
    ui->frmVariogramModelPlaceholder->layout()->addWidget( m_vModelSelector );

}

SGSIMDialog::~SGSIMDialog()
{
    delete ui;
    Application::instance()->logInfo("SGSIMDialog destroyed.");
}
