#include "svdanalysisdialog.h"
#include "ui_svdanalysisdialog.h"
#include "svdfactortree.h"
#include "spectral/svd.h"
#include <QMenu>
#include <QMessageBox>
#include <QProgressDialog>
#include "svdfactorsel/svdfactorsselectiondialog.h"
#include "../widgets/ijgridviewerwidget.h"
#include "../imagejockeyutils.h"
#include "../imagejockeydialog.h"

SVDAnalysisDialog::SVDAnalysisDialog(QWidget *parent) :
    QDialog(parent),
	ui(new Ui::SVDAnalysisDialog),
	m_tree( nullptr ),
    m_deleteTree( false ),
    m_gridWithPhaseForPossibleRFFT( nullptr ),
    m_variableIndexWithPhaseForPossibleRFFT( 0 )
{
    ui->setupUi(this);

    setWindowTitle( "SVD Analysis" );

    //deletes dialog from memory upon user closing it
	this->setAttribute(Qt::WA_DeleteOnClose);

	//positioning the splitter
	ui->splitterMain->setSizes( QList<int>() << 300 << width()-300 );

    //configure factor tree context menu
    m_factorContextMenu = new QMenu( ui->svdFactorTreeView );
    ui->svdFactorTreeView->setContextMenuPolicy( Qt::CustomContextMenu );
    connect(ui->svdFactorTreeView, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(onFactorContextMenu(const QPoint &)));

    //add the grid plot widget to the right pane of the dialog
    m_gridViewerWidget = new IJGridViewerWidget( false, ui->frmRightTop );
    ui->frmRightTop->layout()->addWidget( m_gridViewerWidget );

}

SVDAnalysisDialog::~SVDAnalysisDialog()
{
    delete ui;
	if( m_deleteTree && m_tree ){
		delete m_tree;
		m_tree = nullptr;
	} else if( ! m_deleteTree && m_tree ){
        //Tree of SVDFactors was left in memory
    }
}

void SVDAnalysisDialog::setTree( SVDFactorTree *tree )
{
	m_tree = tree;
	ui->svdFactorTreeView->setModel( m_tree );
	ui->svdFactorTreeView->header()->hide();
    refreshTreeStyle();
}

void SVDAnalysisDialog::setGridWithPhaseForPossibleRFFT(IJAbstractCartesianGrid *grid,
                                                        int indexOfVariableWithPhase)
{
    m_gridWithPhaseForPossibleRFFT = grid;
    m_variableIndexWithPhaseForPossibleRFFT = indexOfVariableWithPhase;
}

void SVDAnalysisDialog::refreshTreeStyle()
{
	ui->svdFactorTreeView->setStyleSheet("QTreeView::branch:has-siblings:!adjoins-item { \
                                   border-image: url(:ijicons32/ijvline32) 0; } \
			QTreeView::branch:has-siblings:adjoins-item { \
                 border-image: url(:ijicons32/ijbmore32) 0; \
			 } \
			\
			 QTreeView::branch:!has-children:!has-siblings:adjoins-item { \
                 border-image: url(:ijicons32/ijbend32) 0; \
			 } \
			\
			 QTreeView::branch:has-children:!has-siblings:closed, \
			 QTreeView::branch:closed:has-children:has-siblings { \
					 border-image: none; \
                     image: url(:ijicons32/ijbclosed32); \
			 } \
			\
			 QTreeView::branch:open:has-children:!has-siblings, \
			 QTreeView::branch:open:has-children:has-siblings  { \
					 border-image: none; \
                     image: url(:ijicons32/ijbopen32); \
             }");
             }

void SVDAnalysisDialog::forcePlotUpdate()
{
    //TODO: find out a more elegant way to make the Qwt Plot redraw (replot() is not working)
    QList<int> oldSizes = ui->splitterMain->sizes();
    QList<int> tmpSizes = oldSizes;
    tmpSizes[0] = oldSizes[0] + 1;
    tmpSizes[1] = oldSizes[1] - 1;
    ui->splitterMain->setSizes( tmpSizes );
    qApp->processEvents();
    ui->splitterMain->setSizes( oldSizes );
    qApp->processEvents();
}

void SVDAnalysisDialog::onFactorContextMenu(const QPoint &mouse_location)
{
    QModelIndex index = ui->svdFactorTreeView->indexAt(mouse_location); //get index to factor under mouse
    m_factorContextMenu->clear(); //remove all context menu actions

    //get all selected items, this may include other items different from the one under the mouse pointer.
    QModelIndexList selected_indexes = ui->svdFactorTreeView->selectionModel()->selectedIndexes();

    //if there is just one selected item.
    if( selected_indexes.size() == 1 ){
        //build context menu for a SVD factor
        if ( index.isValid() ) {
            m_right_clicked_factor = static_cast<SVDFactor*>( index.internalPointer() );
            m_factorContextMenu->addAction("Open in Image Jockey...", this, SLOT(onOpenFactor()));
            if( ! m_right_clicked_factor->hasChildren() )
                m_factorContextMenu->addAction("Factorize further", this, SLOT(onFactorizeFurther()));
            else
                m_factorContextMenu->addAction("Delete children", this, SLOT(onDeleteChildren()));
            m_factorContextMenu->addAction("Save individual factor", this, SLOT(onSaveAFactor()));
        }
    }
    //if there are more than one selected item.
    else {
        SVDFactor* parent_factor = nullptr;
        bool parents_are_the_same = true;
        //for each of the right-clicked factors...
        for( int i = 0; i < selected_indexes.size(); ++i){
            //get its tree widget index
            QModelIndex index = selected_indexes.at(i);
            if( index.isValid() ){
                //retrieve the object pointer
                SVDFactor* right_clicked_factor = static_cast<SVDFactor*>( index.internalPointer() );
                //tests whether all selected factors have the same parent factor
                if( parent_factor && right_clicked_factor->getParent() != parent_factor )
                    parents_are_the_same = false;
                parent_factor = right_clicked_factor->getParent();
            } else {
                parents_are_the_same = false;
            }
        }
        if( parents_are_the_same ){
            m_factorContextMenu->addAction("Aggregate selected factors", this, SLOT(onAggregate()));
        }
    }

    //show the context menu under the mouse cursor.
    if( m_factorContextMenu->actions().size() > 0 )
        m_factorContextMenu->exec(ui->svdFactorTreeView->mapToGlobal(mouse_location));

}

void SVDAnalysisDialog::onFactorizeFurther()
{
    //Compute SVD
    QProgressDialog progressDialog;
    progressDialog.setRange(0,0);
    progressDialog.setLabelText("Computing SVD factors...");
    progressDialog.show();
    QCoreApplication::processEvents();
	spectral::SVD svd = spectral::svd( m_right_clicked_factor->getFactorData() );
    progressDialog.hide();

	//get the grid geometry parameters (useful for displaying)
	double x0 = m_right_clicked_factor->getX0();
	double y0 = m_right_clicked_factor->getY0();
	double z0 = m_right_clicked_factor->getZ0();
	double dx = m_right_clicked_factor->getDX();
	double dy = m_right_clicked_factor->getDY();
	double dz = m_right_clicked_factor->getDZ();

	//get the list with the factor weights (information quantity)
	spectral::array weights = svd.factor_weights();

    //tests whether the factor is factorizable (not fundamental)
    if( weights[0] > 0.999999 ){
        QMessageBox::information( nullptr, "Information", "Selected factor is aready fundamental (not factorizable).");
        m_right_clicked_factor->setType( SVDFactorType::FUNDAMENTAL );
        //update the tree widget to show the factor's new icon
        refreshTreeStyle();
        return;
    } else {
        m_right_clicked_factor->setType( SVDFactorType::GEOLOGICAL );
    }

	//User enters number of SVD factors
	m_numberOfSVDFactorsSetInTheDialog = 0;
    SVDFactorsSelectionDialog * svdfsd = new SVDFactorsSelectionDialog( weights.data(), true, this );
	connect( svdfsd, SIGNAL(numberOfFactorsSelected(int)), this, SLOT(onUserSetNumberOfSVDFactors(int)) );
	int userResponse = svdfsd->exec();
	if( userResponse != QDialog::Accepted )
		return;
	long numberOfFactors = m_numberOfSVDFactorsSetInTheDialog;

	//Get the desired SVD factors
	{
        double splitThreshold = SVDFactor::getSVDFactorTreeSplitThreshold( true );
        m_right_clicked_factor->setChildMergeThreshold( splitThreshold );
        QProgressDialog progressDialog;
		progressDialog.setRange(0,0);
		progressDialog.show();
		for (long i = 0; i < numberOfFactors; ++i) {
			progressDialog.setLabelText("Retrieving SVD factor " + QString::number(i+1) + " of " + QString::number(numberOfFactors) + "...");
			QCoreApplication::processEvents();
			spectral::array factor = svd.factor(i);
            SVDFactor* svdFactor = new SVDFactor( std::move( factor ), i + 1, weights.data()[i], x0, y0, z0, dx, dy, dz,
                                                  splitThreshold );
			m_right_clicked_factor->addChildFactor( svdFactor );
		}
	}

	//update the tree widget
	refreshTreeStyle();
}

void SVDAnalysisDialog::onUserSetNumberOfSVDFactors(int number)
{
	m_numberOfSVDFactorsSetInTheDialog = number;
}

void SVDAnalysisDialog::onOpenFactor()
{
    if( ! m_right_clicked_factor )
        return;
    //calls the Image Jockey dialog
    ImageJockeyDialog *ijd = new ImageJockeyDialog( { m_right_clicked_factor }, this );
    ijd->showMaximized();
}

void SVDAnalysisDialog::onFactorClicked(QModelIndex index)
{
	if( ! index.isValid() )
		return;
	SVDFactor* factor = static_cast<SVDFactor*>( index.internalPointer() );
    m_gridViewerWidget->setFactor( factor );
}

void SVDAnalysisDialog::onSave()
{
    spectral::array *sum = m_tree->getSumOfSelectedFactors();
    emit sumOfFactorsComputed( sum );
}

void SVDAnalysisDialog::onPreview()
{
    SVDFactor* exampleFactor = m_tree->getOneTopLevelFactor( 0 );
    if( ! exampleFactor )
        return;
    spectral::array *sum = m_tree->getSumOfSelectedFactors();
    SVDFactor* factor = new SVDFactor( std::move(*sum), 1, 1,
                                       exampleFactor->getOriginX(),
                                       exampleFactor->getOriginY(),
                                       exampleFactor->getOriginZ(),
                                       exampleFactor->getCellSizeI(),
                                       exampleFactor->getCellSizeJ(),
                                       exampleFactor->getCellSizeK(),
                                       SVDFactor::getSVDFactorTreeSplitThreshold());
    IJGridViewerWidget* wid = new IJGridViewerWidget( true );
    factor->setCustomName( "Sum of selected factors" );
    wid->setFactor( factor );
    wid->show();
}

void SVDAnalysisDialog::onPreviewRFFT()
{
    SVDFactor* exampleFactor = m_tree->getOneTopLevelFactor( 0 );
    if( ! exampleFactor ){
        QMessageBox::critical( nullptr, "Error", "SVDAnalysisDialog::onPreviewRFFT(): No factors.");
        return;
    }

    if( ! m_gridWithPhaseForPossibleRFFT ){
        QMessageBox::critical( nullptr, "Error", "SVDAnalysisDialog::onPreviewRFFT(): A grid with the phase was not supplied.");
        return;
    }

    //create a factor object from the sum of selected factors.
    spectral::array *sum = m_tree->getSumOfSelectedFactors();
    SVDFactor* factor = new SVDFactor( std::move(*sum), 1, 1,
                                       exampleFactor->getOriginX(),
                                       exampleFactor->getOriginY(),
                                       exampleFactor->getOriginZ(),
                                       exampleFactor->getCellSizeI(),
                                       exampleFactor->getCellSizeJ(),
                                       exampleFactor->getCellSizeK(),
                                       SVDFactor::getSVDFactorTreeSplitThreshold());
    factor->setCustomName( "RFFT of sum of factors" );

    //create the array with the input de-shifted and in rectangular form.
    spectral::complex_array dataReady( (spectral::index)exampleFactor->getNI(),
                                       (spectral::index)exampleFactor->getNJ(),
                                       (spectral::index)exampleFactor->getNK() );

    //De-shift frequencies, convert the complex numbers to rectangular form ( a + bi ) and
    //change the scan order from GSLib convention to FFTW3 convention.
    ImageJockeyUtils::prepareToFFTW3reverseFFT( factor,
                                                0,
                                                m_gridWithPhaseForPossibleRFFT,
                                                m_variableIndexWithPhaseForPossibleRFFT,
                                                dataReady );
    //Create the output array.
    spectral::array outputData( (spectral::index)exampleFactor->getNI(),
                                (spectral::index)exampleFactor->getNJ(),
                                (spectral::index)exampleFactor->getNK() );

    //Apply reverse FFT.
    {
        QProgressDialog progressDialog;
        progressDialog.setRange(0,0);
        progressDialog.show();
        progressDialog.setLabelText("Computing RFFT...");
        QCoreApplication::processEvents(); //let Qt repaint widgets
        spectral::backward( outputData, dataReady );
    }

    //Construct a displayable object from the result.
    SVDFactor* factorRFFT = new SVDFactor( std::move(outputData), 1, 1,
                                       exampleFactor->getOriginX(), exampleFactor->getOriginY(), exampleFactor->getOriginZ(),
                                       exampleFactor->getCellSizeI(), exampleFactor->getCellSizeJ(), exampleFactor->getCellSizeK(),
                                           SVDFactor::getSVDFactorTreeSplitThreshold());

    //Opens the viewer.
    IJGridViewerWidget* ijgvw = new IJGridViewerWidget( true );
    factorRFFT->setCustomName("Reverse FFT");
    ijgvw->setFactor( factorRFFT );
    ijgvw->show();
}

void SVDAnalysisDialog::onDeleteChildren()
{
    m_right_clicked_factor->deleteChildren();
    //update the tree widget
    refreshTreeStyle();
}

void SVDAnalysisDialog::onAggregate()
{
    //upon call of this slot, it is assumed the parent factor is the same for all selected factors
    //get all selected items, this may include other items different from the one under the mouse pointer.
    QModelIndexList selected_indexes = ui->svdFactorTreeView->selectionModel()->selectedIndexes();
    SVDFactor* parent_factor = nullptr;
    std::vector<SVDFactor*> selected_factors;
    //for each of the right-clicked factors...
    for( int i = 0; i < selected_indexes.size(); ++i){
        //get its tree widget index
        QModelIndex index = selected_indexes.at(i);
        if( index.isValid() ){
            //retrieve the object pointer
            SVDFactor* right_clicked_factor = static_cast<SVDFactor*>( index.internalPointer() );
            //get the parent factor
            parent_factor = right_clicked_factor->getParent();
            //update the list of selected factors
            selected_factors.push_back( right_clicked_factor );
        }
    }
    saveTreeUIState();
    //disable the model to prevent crashes during aggregation
    ui->svdFactorTreeView->setModel( nullptr );
    //aggregate the selected factors
    parent_factor->aggregate( selected_factors );
    //re-enabling the tree widget model
    //TODO: restore tree state
    ui->svdFactorTreeView->setModel( m_tree );
    restoreTreeUIState();
}

void SVDAnalysisDialog::onSaveAFactor()
{
    spectral::array *oneFactorData = new spectral::array( m_right_clicked_factor->getFactorData() );
    //reuse the signal to save a single factor data
    emit sumOfFactorsComputed( oneFactorData );
}

void SVDAnalysisDialog::saveTreeUIState()
{
    m_listForTreeStateKeeping.clear();
    // prepare list
    // PS: getPersistentIndexList() function is a simple `return this->persistentIndexList()` from TreeModel model class
    foreach (QModelIndex index, m_tree->getPersistentIndexList() )
    {
        if ( ui->svdFactorTreeView->isExpanded(index))
        {
            m_listForTreeStateKeeping << index.data(Qt::DisplayRole).toString(); //DisplayRole == item label (may use UserRole with an unique ID to assure uniqueness)
        }
    }
}


void SVDAnalysisDialog::restoreTreeUIState()
{
    foreach (QString item, m_listForTreeStateKeeping)
    {
        // search `item` data in model
        foreach (QModelIndex model_item, m_tree->getIndexList( QModelIndex() )){
            if ( model_item.data( Qt::DisplayRole ).toString() == item ){
                ui->svdFactorTreeView->setExpanded(model_item, true);
                break; //end internal loop since the item was found.
            }
        }
    }
}
