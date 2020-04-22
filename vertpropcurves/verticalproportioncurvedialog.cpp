#include "verticalproportioncurvedialog.h"
#include "ui_verticalproportioncurvedialog.h"

#include "domain/application.h"
#include "domain/projectcomponent.h"
#include "domain/attribute.h"
#include "domain/project.h"
#include "domain/categorydefinition.h"
#include "domain/datafile.h"
#include "domain/categorypdf.h"
#include "widgets/fileselectorwidget.h"
#include "widgets/variableselector.h"
#include "vertpropcurves/verticalproportioncurvesplot.h"
#include <QDragEnterEvent>
#include <QMimeData>

VerticalProportionCurveDialog::VerticalProportionCurveDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VerticalProportionCurveDialog),
    m_fallbackPDF( nullptr )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    //enables this dialog as a drop destination in DnD operations
    setAcceptDrops(true);

    //add the fallback PDF file selector
    m_cmbFallBackPDF = new FileSelectorWidget( FileSelectorType::PDFs );
    ui->frmCmbFallbackPDF->layout()->addWidget( m_cmbFallBackPDF );
    connect( m_cmbFallBackPDF, SIGNAL(fileSelected( File* )), this, SLOT(onFallbackPDFChanged( File* )) );
    m_cmbFallBackPDF->onSelection( 0 ); //to evoke the onFallbackPDFChanged() slot

    //the top and base horizons selectors
    m_cmbTopHorizon = new FileSelectorWidget( FileSelectorType::CartesianGrids2D );
    ui->frmCmbTopHorizon->layout()->addWidget( m_cmbTopHorizon );
    m_cmbBaseHorizon = new FileSelectorWidget( FileSelectorType::CartesianGrids2D );
    ui->frmCmbBaseHorizon->layout()->addWidget( m_cmbBaseHorizon );

    //top and base horizons' variables with the depth values
    m_cmbTopVariable = new VariableSelector( );
    ui->frmCmbTopVariable->layout()->addWidget( m_cmbTopVariable );
    m_cmbBaseVariable = new VariableSelector( );
    ui->frmCmbBaseVariable->layout()->addWidget( m_cmbBaseVariable );
    connect( m_cmbTopHorizon, SIGNAL(dataFileSelected( DataFile* )),
             m_cmbTopVariable, SLOT(onListVariables( DataFile* ) ) );
    connect( m_cmbBaseHorizon, SIGNAL(dataFileSelected( DataFile* )),
             m_cmbBaseVariable, SLOT(onListVariables( DataFile* ) ) );

    //calling the onSelection slot on the file selectors causes the variable comboboxes to update,
    //so they show up populated otherwise the user is required to choose another file and then back
    //to the first file if the desired sample file happens to be the first one in the list.
    m_cmbTopHorizon->onSelection( 0 );
    m_cmbBaseHorizon->onSelection( 0 );

    //add the widget used to edit the calibration curves
    m_VPCPlot = new VerticalProportionCurvesPlot( );
    ui->frmDisplayVPC->layout()->addWidget( m_VPCPlot );

    setWindowTitle( "Create vertical proportion curves from data." );
}

VerticalProportionCurveDialog::~VerticalProportionCurveDialog()
{
    delete ui;
    Application::instance()->logInfo("VerticalProportionCurveDialog destroyed.");
}

void VerticalProportionCurveDialog::dragEnterEvent(QDragEnterEvent *e)
{
    e->acceptProposedAction();
}

void VerticalProportionCurveDialog::dragMoveEvent(QDragMoveEvent *e)
{
    QPoint eventPos = e->pos();

    //the variables list accepts drops if they come from somewhere other than lstVariables itself.
    if( ui->lstVariables->geometry().contains( eventPos ) && e->source() != ui->lstVariables ){
        m_dragOrigin = DragOrigin::FROM_ELSEWHERE;
        e->accept();
    //the trash bin label accepts drops if they come from the variables list.
    }else if( ui->lblTrash->geometry().contains( eventPos ) && e->source() == ui->lstVariables ){
        m_dragOrigin = DragOrigin::FROM_LIST_OF_VARIABLES;
        e->accept();
    }else {
        e->ignore();
        return;
    }

}

void VerticalProportionCurveDialog::dropEvent(QDropEvent *e)
{
    //user may be dropping data files to add to the project
    if (e->mimeData()->hasUrls()) {
        foreach (const QUrl &url, e->mimeData()->urls()) {
            QString fileName = url.toLocalFile();
            Application::instance()->addDataFile( fileName );
        }
        return;
    }

    //otherwise, they are project objects
    //inform user that an object was dropped
    QString object_locator = e->mimeData()->text();
    Application::instance()->logInfo("VerticalProportionCurveDialog::dropEvent(): Dropped object locator: " + object_locator);
    ProjectComponent* object = Application::instance()->getProject()->findObject( object_locator );
    if( ! object ){
        Application::instance()->logError("VerticalProportionCurveDialog::dropEvent(): object not found. Drop operation failed.");
    } else {
        Application::instance()->logInfo("VerticalProportionCurveDialog::dropEvent(): Found object: " + object->getName());
        if( object->isAttribute() ){
            Attribute* attributeAspect = dynamic_cast<Attribute*>( object );
            if( attributeAspect->isCategorical() ){
                //if dragging objects into the dialog
                if( m_dragOrigin == DragOrigin::FROM_ELSEWHERE ){
                    tryToAddAttribute( attributeAspect );
                } else { //if dragging objects from the dialog's list of variables to the trash bin
                    tryToRemoveAttribute( attributeAspect );
                }
            }else
                Application::instance()->logError("VerticalProportionCurveDialog::dropEvent(): attribute is not categorical.");
        } else
            Application::instance()->logError("VerticalProportionCurveDialog::dropEvent(): object is not an attribute.");
    }
}

void VerticalProportionCurveDialog::onRun()
{

}

void VerticalProportionCurveDialog::onSave()
{

}

void VerticalProportionCurveDialog::onFallbackPDFChanged( File *pdf )
{
    m_fallbackPDF = dynamic_cast< CategoryPDF* >( pdf );
    if( ! m_fallbackPDF )
        Application::instance()->logError("VerticalProportionCurveDialog::onFallbackPDFChanged(): passed file is null or is not of CategoryPDF class.");

    //clear the list with categorical variables
    m_categoricalAttributes.clear();
    updateVariablesList();

    //redefine the number of curves in the curves plot widget.
    if( m_fallbackPDF->getCategoryDefinition() ) {
        //m_VPCPlot->setNumberOfCurves( m_fallbackPDF->getCategoryDefinition()->getCategoryCount() );
        CONTINUE_HERE_THE_CALL_BELOW_IS_CRASHING;
        m_VPCPlot->setNumberOfCurves( 3 );
    } else {
        m_VPCPlot->setNumberOfCurves( 2 );
        Application::instance()->logError("VerticalProportionCurveDialog::onFallbackPDFChanged(): the fallback PDF's CategoryDefinition is null.");
    }
}


void VerticalProportionCurveDialog::tryToAddAttribute(Attribute *attribute)
{
    CategoryDefinition* CDofFirst = nullptr;

    if( ! m_fallbackPDF ) {
        Application::instance()->logError( "VerticalProportionCurveDialog::tryToAddAttribute(): fallback PDF not provided.", true );
        return;
    } else
        CDofFirst = m_fallbackPDF->getCategoryDefinition();

    if( ! CDofFirst ) {
        Application::instance()->logError( "VerticalProportionCurveDialog::tryToAddAttribute(): could not retrieve the category definition associated with the fallback PDF.", true );
        return;
    }

    //get info from the attribute to be added
    DataFile* myParent = dynamic_cast<DataFile*>( attribute->getContainingFile() );
    CategoryDefinition* myCD = myParent->getCategoryDefinition( attribute );

    //the category defininion must be the same as the fallback PDF's
    if( CDofFirst != myCD ){
        Application::instance()->logError( "VerticalProportionCurveDialog::tryToAddAttribute(): all attributes must be associated to the same categorical definition of the fallback PDF.", true );
        return;
    }

    //it can't be added twice
    for( Attribute* at : m_categoricalAttributes ){
        if( at == attribute ){
            Application::instance()->logError( "VerticalProportionCurveDialog::tryToAddAttribute(): the attribute has already been added.", true );
            return;
        }
    }

    //add the dragged attribute to the bookkeeping list
    m_categoricalAttributes.push_back( attribute );

    updateVariablesList();
}

void VerticalProportionCurveDialog::tryToRemoveAttribute(Attribute *attribute)
{
    std::vector<Attribute*>::iterator it;

    //add the dragged attribute from the bookkeeping list
    it = std::find(m_categoricalAttributes.begin(), m_categoricalAttributes.end(), attribute);
    if (it != m_categoricalAttributes.end())
        m_categoricalAttributes.erase( it );
    else
        Application::instance()->logError("VerticalProportionCurveDialog::tryToRemoveAttribute(): object pointer not found.  The dialog may be in an inconsistent state.");

    updateVariablesList();
}

void VerticalProportionCurveDialog::updateVariablesList()
{
    ui->lstVariables->clear();
    for( Attribute* attribute : m_categoricalAttributes ){
        QListWidgetItem* item = new QListWidgetItem( attribute->getIcon(),
                                                     attribute->getName() + " (" + attribute->getContainingFile()->getName() + ")" );
        item->setData( Qt::UserRole, reinterpret_cast<uint64_t>( attribute ) );
        ui->lstVariables->addItem( item );
    }
    //readjusts the list widget's width to fit the content.
    ui->lstVariables->setMinimumWidth( ui->lstVariables->sizeHintForColumn(0) + 5 );
}
