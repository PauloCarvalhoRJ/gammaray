#include "verticalproportioncurvedialog.h"
#include "ui_verticalproportioncurvedialog.h"

#include "domain/application.h"
#include "domain/attribute.h"
#include "domain/cartesiangrid.h"
#include "domain/categorydefinition.h"
#include "domain/categorypdf.h"
#include "domain/datafile.h"
#include "domain/project.h"
#include "domain/projectcomponent.h"
#include "domain/segmentset.h"
#include "domain/auxiliary/verticalproportioncurvemaker.h"
#include "geometry/intersectionfinder.h"
#include "vertpropcurves/verticalproportioncurvesplot.h"
#include "widgets/fileselectorwidget.h"
#include "widgets/variableselector.h"

#include <QDragEnterEvent>
#include <QInputDialog>
#include <QMimeData>
#include <QMessageBox>

VerticalProportionCurveDialog::VerticalProportionCurveDialog(VerticalProportionCurve *vpc, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::VerticalProportionCurveDialog),
    m_fallbackPDF( nullptr ),
    m_currentVPC( vpc )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    //enables this dialog as a drop destination in DnD operations
    setAcceptDrops(true);

    //add the widget used to edit the calibration curves
    m_VPCPlot = new VerticalProportionCurvesPlot( );
    ui->frmDisplayVPC->layout()->addWidget( m_VPCPlot );

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

    setWindowTitle( "Create vertical proportion curves from data." );

    if( m_currentVPC ) { //if the user wants to view/edit an existing curve
        ui->frmLeftPane->hide();
        ui->frmConfig->hide();
        setWindowTitle( "Review vertical proportion curve: " + m_currentVPC->getName() );
        m_currentVPC->readFromFS();
        updatePlotWithCurrentVPC();
    }
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
    std::vector< VerticalProportionCurve > curves;

    //for each categorical attribute
    for( Attribute* at : m_categoricalAttributes ){
        //get its parent file
        File* parentFile = at->getContainingFile();
        if( parentFile->getTypeName() == "SEGMENTSET" ){
            //compute the vertical proportion curve
            VerticalProportionCurve vpc = computeProportionsForASegmentSet( at );
            if( vpc.isEmpty() )
                Application::instance()->logWarn("VerticalProportionCurveDialog::onRun(): VPC for variable "
                                                 + at->getName() + " of file " + parentFile->getName() + " failed.");
            else
                curves.push_back( vpc );
        } else {
            Application::instance()->logWarn("VerticalProportionCurveDialog::onRun(): data files of type " +
                                              parentFile->getTypeName() + " not currently supported.  Variable "
                                             + at->getName() + " of file " + parentFile->getName() + " ignored.");
        }
    }

    if( ! curves.empty() ) {

        //Get the category definition of the case.
        CategoryDefinition *cd = m_fallbackPDF->getCategoryDefinition();

        //make a mean VPC from the individual VPCs for each categorical variable.
        if( m_currentVPC && ! m_currentVPC->hasParent() ) //do not delete the object if it has been added to the project
            delete m_currentVPC;
        m_currentVPC = new VerticalProportionCurve( "", cd->getName() );
        bool ok = m_currentVPC->setAsMeanOf( curves );
        if( ok )
            updatePlotWithCurrentVPC();
    }

}

void VerticalProportionCurveDialog::onSave()
{
    if( ! m_currentVPC ){
        QMessageBox::critical( this, "Error", QString("No curve to save. Run computation at least once."));
        return;
    }

    updateCurrentVPCWithPlot();

    if( m_currentVPC->hasParent() )
        saveExisting();
    else
        saveNew();
}

void VerticalProportionCurveDialog::onFallbackPDFChanged( File *pdf )
{
    m_fallbackPDF = dynamic_cast< CategoryPDF* >( pdf );
    if( ! m_fallbackPDF )
        Application::instance()->logError("VerticalProportionCurveDialog::onFallbackPDFChanged(): passed file is null or is not of CategoryPDF class.");

    //if there is a current VPC, but is has not been added
    //to the project, delete it.
    if( m_currentVPC && ! m_currentVPC->hasParent() ){
        delete m_currentVPC;
        m_currentVPC = nullptr;
    }

    //clear the list with categorical variables
    m_categoricalAttributes.clear();
    updateVariablesList();

    //redefine the number of curves in the curves plot widget.
    if( m_fallbackPDF->getCategoryDefinition() ) {
        m_fallbackPDF->loadPairs();
        m_fallbackPDF->getCategoryDefinition()->loadQuintuplets();
        updateCurvesOfPlot( );
    } else {
        m_VPCPlot->setNumberOfCurves( 2 );
        Application::instance()->logError("VerticalProportionCurveDialog::onFallbackPDFChanged(): the fallback PDF's CategoryDefinition is null.");
    }
}

void VerticalProportionCurveDialog::onEditCurves()
{
    m_VPCPlot->setEditable( ui->btnEditCurves->isChecked() );
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
        item->setData( Qt::UserRole, QVariant::fromValue( reinterpret_cast<uint64_t>( attribute ) ) );
        ui->lstVariables->addItem( item );
    }
    //readjusts the list widget's width to fit the content.
    ui->lstVariables->setMinimumWidth( ui->lstVariables->sizeHintForColumn(0) + 5 );
}

void VerticalProportionCurveDialog::updateCurvesOfPlot( )
{
    //The colors of the categories will be used to fill the areas in the plot.
    CategoryDefinition *cd = nullptr;
    {
        //If there is a curve already...
        if( m_currentVPC )
            //...use the one defined in it.
            cd = m_currentVPC->getAssociatedCategoryDefinition();
        //If the category definition is still unknown...
        if( ! cd )
            //...use whatever selected by the user
            cd = m_fallbackPDF->getCategoryDefinition();
        //If we still don't have a category definition...
        if( ! cd )
            //...give up.
            return;
        //Load the data of the category definition.
        cd->loadQuintuplets();
    }

    int nCategories = cd->getCategoryCount();

    //for categorical variables the calibration curves separate the categories, thus -1.
    m_VPCPlot->setNumberOfCurves( nCategories-1 );

    //if we don't have a vpc...
    if( ! m_currentVPC ){
        //show the curves according to default proportions from the fallback PDF
        double curveBase = 0.0;
        for( int i = 0; i < nCategories; ++i ){
            m_VPCPlot->fillColor( Util::getGSLibColor( cd->getCategoryColorByCode( m_fallbackPDF->get1stValue( i ) ) ) ,
                                           i-1,
                                           cd->getCategoryNameByCode( m_fallbackPDF->get1stValue( i ) ) );
            if( i > 0 )
                m_VPCPlot->setCurveBase( i-1, curveBase * 100.0 );
            curveBase += m_fallbackPDF->get2ndValue( i );
        }
    } else { // if we have a vpc, provide the filled areas as required by its category definition.
        for( int i = 0; i < nCategories; ++i ){
            m_VPCPlot->fillColor( Util::getGSLibColor( cd->getColorCode( i ) ) ,
                                           i-1,
                                           cd->getCategoryName( i ) );
            if( i > 0 )
                m_VPCPlot->setCurveBase( i-1, i * 100.0 / nCategories ); //make up a base value for the curves
        }
    }

    m_VPCPlot->updateFillAreas();
}

void VerticalProportionCurveDialog::saveNew()
{
    QString suggestedName = "VPC_from_";
    suggestedName += m_cmbBaseVariable->getSelectedVariableName();
    suggestedName += "_to_";
    suggestedName += m_cmbTopVariable->getSelectedVariableName();
    suggestedName += "_resol_";
    suggestedName += QString::number( static_cast<int>( ui->dblSpinResolutionPercent->value() ) );
    suggestedName += "_window_";
    suggestedName += QString::number( static_cast<int>( ui->dblSpinWindowPercent->value() ) );

    //open file rename dialog
    bool ok;
    QString newName = QInputDialog::getText(this, "Name the new file",
                                             "New vertical proportion curve name:", QLineEdit::Normal,
                                             suggestedName, &ok);

    //do saving steps for a new VPC file.
    if (ok && !newName.isEmpty()){

        //sets the path.
        m_currentVPC->setPath( Application::instance()->getProject()->getPath() + '/' + newName );

        //Give meaningful column names for the GEO-EAS format.
        //Without this step, the system would create meaningless default names in the file.
        {
            //the leading relative depth
            Attribute* at = new Attribute( "Relative depth", 1 );
            m_currentVPC->addChild( at );

            //the proportion of each facies follow on
            CategoryDefinition *cd = m_fallbackPDF->getCategoryDefinition();
            int nCategories = cd->getCategoryCount();
            for( int i = 0; i < nCategories; ++i ){
                Attribute* at = new Attribute( "Proportion of " + cd->getCategoryName(i), 2 + i );
                m_currentVPC->addChild( at );
            }
        }

        //save the file data.
        m_currentVPC->writeToFS();

        //save the metadata file.
        m_currentVPC->updateMetaDataFile();

        //add the project file to the project
        Application::instance()->getProject()->addVerticalProportionCurve( m_currentVPC );

        //refresh the project tree widget
        Application::instance()->refreshProjectTree();

    }
}

void VerticalProportionCurveDialog::saveExisting()
{
    //save the file data (updates).
    m_currentVPC->writeToFS();
}

void VerticalProportionCurveDialog::updatePlotWithCurrentVPC()
{
    if( ! m_currentVPC )
        return;

    //sets the number of samples (number of entries).
    m_VPCPlot->setNumberOfPoints( m_currentVPC->getEntriesCount() );
    updateCurvesOfPlot();

    //the last curve is ignored because the last curve would be the right border
    //of the chart.
    for( int iCurve = 0; iCurve < m_currentVPC->getProportionsCount()-1; ++iCurve )
        m_VPCPlot->setCurveValues( iCurve, m_currentVPC->getNthCumulativeProportions( iCurve ), 100.0 );

    //update the painted areas between the curves.
    m_VPCPlot->updateFillAreas();
}

void VerticalProportionCurveDialog::updateCurrentVPCWithPlot()
{
    CategoryDefinition* cd = m_currentVPC->getAssociatedCategoryDefinition();
    for( int iEntry = 0; iEntry < m_currentVPC->getEntriesCount(); ++iEntry ){
        for( int iCategory = 0; iCategory < cd->getCategoryCount(); ++iCategory ){

            int categoryCode = cd->getCategoryCode( iCategory );

            //The proportion of a category is a difference between two curves.
            double proportion = 0.0;
            if( iCategory == 0 )                             //first category
                proportion =   m_VPCPlot->getCurveValue( iCategory,   iEntry )
                             - 0.0; //the "curve" to the left is the chart left border, that is, zero.
            else if( iCategory+1 == cd->getCategoryCount() ) //last category
                proportion =   100.0 //the "curve" to the right is the chart right border, that is, 100.0.
                             - m_VPCPlot->getCurveValue( iCategory-1, iEntry );
            else                                             //categories in the middle
                proportion =   m_VPCPlot->getCurveValue( iCategory,   iEntry )
                             - m_VPCPlot->getCurveValue( iCategory-1, iEntry );
            proportion /= 100.0; //unitize back to 0.0-1.0

            m_currentVPC->setProportion( iEntry, categoryCode, proportion );

        }
    }
}

VerticalProportionCurve VerticalProportionCurveDialog::computeProportionsForASegmentSet(Attribute *at)
{
    //Get and load the segment set
    //We can assume the attribute's parent data file is a segment set.
    SegmentSet* ss = dynamic_cast< SegmentSet* >( at->getContainingFile() );
    ss->loadData();

    //Get and load the horizons
    CartesianGrid* cgTop  = dynamic_cast< CartesianGrid* >( m_cmbTopHorizon->getSelectedFile() );
    CartesianGrid* cgBase = dynamic_cast< CartesianGrid* >( m_cmbBaseHorizon->getSelectedFile() );
    if( !cgTop or !cgBase ){
        Application::instance()->logError( "VerticalProportionCurveDialog::computeProportionsForASegmentSet(): "
                                           "One horizon is missing or is not a Cartesian grid.", true );
        return VerticalProportionCurve("", "");
    }
    cgTop->loadData();
    cgBase->loadData();

    //find the depth at which the segment set intersects the top horizon
    double top = std::numeric_limits<double>::quiet_NaN();
    {
        IntersectionFinder intFinder;
        intFinder.initWithSurface( cgTop, m_cmbTopVariable->getSelectedVariableGEOEASIndex()-1 );
        std::vector< Vector3D > intersections = intFinder.getIntersections( ss );
        if( intersections.empty() ){
            Application::instance()->logWarn("VerticalProportionCurveDialog::computeProportionsForASegmentSet(): " +
                                             ss->getName() + " does not intersect top horizon.  Using its topmost depth.");
            top = std::max( ss->max( ss->getZindex()-1 ), ss->max( ss->getZFinalIndex()-1 ) );
        } else if( intersections.size() > 1 ){
            Application::instance()->logWarn("VerticalProportionCurveDialog::computeProportionsForASegmentSet(): " +
                                              ss->getName() + " intersects top horizon more than once and was ignored.");
            return VerticalProportionCurve("", "");
        } else {
            top = intersections[0].z;
            Application::instance()->logInfo( ss->getName() + " intersects " + m_cmbTopVariable->getSelectedVariableName() +
                                              " top horizon at z = " + QString::number( top ) + ".");
        }
    }

    //find the depth at which the segment set intersects the base horizon
    double base = std::numeric_limits<double>::quiet_NaN();
    {
        IntersectionFinder intFinder;
        intFinder.initWithSurface( cgBase, m_cmbBaseVariable->getSelectedVariableGEOEASIndex()-1 );
        std::vector< Vector3D > intersections = intFinder.getIntersections( ss );
        if( intersections.empty() ){
            Application::instance()->logWarn("VerticalProportionCurveDialog::computeProportionsForASegmentSet(): " +
                                             ss->getName() + " does not intersect base horizon.  Using its bottommost depth.");
            base = std::min( ss->min( ss->getZindex()-1 ), ss->min( ss->getZFinalIndex()-1 ) );
        } else if( intersections.size() > 1 ){
            Application::instance()->logWarn("VerticalProportionCurveDialog::computeProportionsForASegmentSet(): " +
                                              ss->getName() + " intersects base horizon more than once and was ignored.");
            return VerticalProportionCurve("", "");
        } else {
            base = intersections[0].z;
            Application::instance()->logInfo( ss->getName() + " intersects " + m_cmbBaseVariable->getSelectedVariableName() +
                                              " base horizon at z = " + QString::number( base ) + ".");
        }
    }

    //sanity checks
    if( base > top ) {
        Application::instance()->logWarn("VerticalProportionCurveDialog::computeProportionsForASegmentSet(): "
                                         "intersection base depth is higher than intersection top depth. "
                                         "This is likely due to swapped base and top horizons.  Ignored.");
        return VerticalProportionCurve("", "");
    }

    //compute and return the curves
    VerticalProportionCurveMaker< SegmentSet > vpcMaker( ss, at->getAttributeGEOEASgivenIndex()-1 );
    return vpcMaker.makeInDepthInterval( ui->dblSpinResolutionPercent->value() / 100.0,
                                  ui->dblSpinWindowPercent->value() / 100.0,
                                  top,
                                  base,
                                  *m_fallbackPDF );
}
