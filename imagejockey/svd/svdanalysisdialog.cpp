#include "svdanalysisdialog.h"
#include "ui_svdanalysisdialog.h"
#include "domain/application.h"
#include "svdfactortree.h"
#include "spectral/svd.h"
#include <QMenu>
#include <QProgressDialog>
#include "svdfactorsel/svdfactorsselectiondialog.h"
#include "../imagejockeygridplot.h"

SVDAnalysisDialog::SVDAnalysisDialog(QWidget *parent) :
    QDialog(parent),
	ui(new Ui::SVDAnalysisDialog),
	m_tree( nullptr ),
	m_deleteTree( false )
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

	//add a grid plot widget to the right pane of the dialog
	m_gridPlot = new ImageJockeyGridPlot();
	ui->frmRightTop->layout()->addWidget( m_gridPlot );
}

SVDAnalysisDialog::~SVDAnalysisDialog()
{
    Application::instance()->logInfo("SVDAnalysisDialog destroyed.");
    delete ui;
	if( m_deleteTree && m_tree ){
		delete m_tree;
		m_tree = nullptr;
		Application::instance()->logInfo("Tree of SVDFactors destroyed.");
	} else if( ! m_deleteTree && m_tree ){
		Application::instance()->logInfo("Tree of SVDFactors was left in memory.");
	}
}

void SVDAnalysisDialog::setTree( SVDFactorTree *tree )
{
	m_tree = tree;
	ui->svdFactorTreeView->setModel( m_tree );
	ui->svdFactorTreeView->header()->hide();
	refreshTreeStyle();
}

void SVDAnalysisDialog::refreshTreeStyle()
{
	ui->svdFactorTreeView->setStyleSheet("QTreeView::branch:has-siblings:!adjoins-item { \
								   border-image: url(:icons32/vline32) 0; } \
			QTreeView::branch:has-siblings:adjoins-item { \
				 border-image: url(:icons32/bmore32) 0; \
			 } \
			\
			 QTreeView::branch:!has-children:!has-siblings:adjoins-item { \
				 border-image: url(:icons32/bend32) 0; \
			 } \
			\
			 QTreeView::branch:has-children:!has-siblings:closed, \
			 QTreeView::branch:closed:has-children:has-siblings { \
					 border-image: none; \
					 image: url(:icons32/bclosed32); \
			 } \
			\
			 QTreeView::branch:open:has-children:!has-siblings, \
			 QTreeView::branch:open:has-children:has-siblings  { \
					 border-image: none; \
					 image: url(:icons32/bopen32); \
			 }");
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
			m_factorContextMenu->addAction("Open...", this, SLOT(onOpenFactor()));
			m_factorContextMenu->addAction("Factorize further", this, SLOT(onFactorizeFurther()));
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

	//get the list with the factor weights (information quantity)
	spectral::array weights = svd.factor_weights();
	Application::instance()->logInfo("ImageJockeyDialog::onSVD(): " + QString::number( weights.data().size() ) + " factor(s) were found.");

	//User enters number of SVD factors
	m_numberOfSVDFactorsSetInTheDialog = 0;
	SVDFactorsSelectionDialog * svdfsd = new SVDFactorsSelectionDialog( weights.data(), this );
	connect( svdfsd, SIGNAL(numberOfFactorsSelected(int)), this, SLOT(onUserSetNumberOfSVDFactors(int)) );
	int userResponse = svdfsd->exec();
	if( userResponse != QDialog::Accepted )
		return;
	long numberOfFactors = m_numberOfSVDFactorsSetInTheDialog;

	//Get the desired SVD factors
	{
		QProgressDialog progressDialog;
		progressDialog.setRange(0,0);
		progressDialog.show();
		for (long i = 0; i < numberOfFactors; ++i) {
			progressDialog.setLabelText("Retrieving SVD factor " + QString::number(i+1) + " of " + QString::number(numberOfFactors) + "...");
			QCoreApplication::processEvents();
			spectral::array factor = svd.factor(i);
			SVDFactor* svdFactor = new SVDFactor( std::move( factor ), i + 1, weights.data()[i] );
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

}
