#include "svdanalysisdialog.h"
#include "ui_svdanalysisdialog.h"
#include "domain/application.h"

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
