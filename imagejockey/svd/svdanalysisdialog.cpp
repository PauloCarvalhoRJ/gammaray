#include "svdanalysisdialog.h"
#include "ui_svdanalysisdialog.h"
#include "domain/application.h"
#include "svdfactortree.h"
#include "spectral/svd.h"
#include <QMenu>
#include <QProgressDialog>

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
            m_factorContextMenu->addAction("Factorize further", this, SLOT(onFactorizeFurther()));
        }
    }

    //show the context menu under the mouse cursor.
    if( m_factorContextMenu->actions().size() > 0 )
        m_factorContextMenu->exec(ui->svdFactorTreeView->mapToGlobal(mouse_location));

}

void SVDAnalysisDialog::onFactorizeFurther()
{
    SVDFactor* factor = m_right_clicked_factor;


    //Compute SVD
    QProgressDialog progressDialog;
    progressDialog.setRange(0,0);
    progressDialog.setLabelText("Computing SVD factors...");
    progressDialog.show();
    QCoreApplication::processEvents();
    spectral::SVD svd = spectral::svd( factor->getFactorData() );
    progressDialog.hide();

}
