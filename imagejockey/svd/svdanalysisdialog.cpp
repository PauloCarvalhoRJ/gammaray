#include "svdanalysisdialog.h"
#include "ui_svdanalysisdialog.h"
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

    connect( ui->dblSpinColorScaleMin, SIGNAL(valueChanged(double)), m_gridPlot, SLOT(setColorScaleMin(double)));
    connect( ui->dblSpinColorScaleMax, SIGNAL(valueChanged(double)), m_gridPlot, SLOT(setColorScaleMax(double)));
    connect( ui->cmbColorScale, SIGNAL(currentIndexChanged(int)), this, SLOT(onCmbColorScaleValueChanged(int)));

    connect( ui->cmbPlane, SIGNAL(currentIndexChanged(int)), this, SLOT(onCmbPlaneChanged(int)));
    connect( ui->spinSlice, SIGNAL(valueChanged(int)), this, SLOT(onSpinSliceChanged(int)));
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

void SVDAnalysisDialog::adjustColorTableWidgets(int cmbIndex)
{
    //get the current selected factor
    QModelIndex modelIndex = ui->svdFactorTreeView->currentIndex();
    if( ! modelIndex.isValid() )
        return;
    SVDFactor* factor = static_cast<SVDFactor*>( modelIndex.internalPointer() );

    //adjust the color table widgets
    double max = factor->getMaxValue();
    double min = factor->getMinValue();
    double valueMin = ui->dblSpinColorScaleMin->value();
    double valueMax = ui->dblSpinColorScaleMax->value();
    if( cmbIndex == 0 ) {//currently log, switching to linear
        if( std::isnan( valueMax ) )
            valueMax = max;
        else
            valueMax = std::pow( 10, valueMax );
        if( std::isnan( valueMin ) )
            valueMin = min;
        else
            valueMin = std::pow( 10, valueMin );
    }
    if( cmbIndex == 1 ){ //currently linear, switching to log
        max = std::log10( max );
        if( min < 0)
            min = 1e-6;
        min = std::log10( min );
        valueMax = std::log10( valueMax );
        if( valueMin < 0)
            valueMin = 1e-6;
        valueMin = std::log10( valueMin );
    }
    ui->dblSpinColorScaleMax->setMaximum( max );
    ui->dblSpinColorScaleMin->setMaximum( max );
    ui->dblSpinColorScaleMax->setMinimum( min );
    ui->dblSpinColorScaleMin->setMinimum( min );
    ui->dblSpinColorScaleMax->setValue( valueMax );
    ui->dblSpinColorScaleMin->setValue( valueMin );

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

	//get the grid geometry parameters (useful for displaying)
	double x0 = m_right_clicked_factor->getX0();
	double y0 = m_right_clicked_factor->getY0();
	double z0 = m_right_clicked_factor->getZ0();
	double dx = m_right_clicked_factor->getDX();
	double dy = m_right_clicked_factor->getDY();
	double dz = m_right_clicked_factor->getDZ();

	//get the list with the factor weights (information quantity)
	spectral::array weights = svd.factor_weights();

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
			SVDFactor* svdFactor = new SVDFactor( std::move( factor ), i + 1, weights.data()[i], x0, y0, z0, dx, dy, dz );
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

void SVDAnalysisDialog::onFactorClicked(QModelIndex index)
{
	if( ! index.isValid() )
		return;
	SVDFactor* factor = static_cast<SVDFactor*>( index.internalPointer() );
	m_gridPlot->setSVDFactor( factor );
	double min = factor->getMinValue();
	double max = factor->getMaxValue();
	m_gridPlot->setColorScaleMin( min );
	m_gridPlot->setColorScaleMax( max );
	ui->dblSpinColorScaleMax->setMaximum( max );
	ui->dblSpinColorScaleMax->setMinimum( min );
	ui->dblSpinColorScaleMax->setValue( max );
	ui->dblSpinColorScaleMin->setMaximum( max );
	ui->dblSpinColorScaleMin->setMinimum( min );
	ui->dblSpinColorScaleMin->setValue( min );
    ui->spinSlice->setMaximum( factor->getCurrentPlaneNumberOfSlices()-1 ); //1st == 0; last == total-1
    if( ui->cmbColorScale->currentIndex() == 1 )
        adjustColorTableWidgets( 1 );
}

void SVDAnalysisDialog::onCmbColorScaleValueChanged(int index)
{
    //change color scaling
    if( index == 0 )
        m_gridPlot->setColorScaleForSVDFactor( ColorScaleForSVDFactor::LINEAR );
    if( index == 1 )
        m_gridPlot->setColorScaleForSVDFactor( ColorScaleForSVDFactor::LOG );

    //adjust the color table max/min spin widgets
    adjustColorTableWidgets( index );

    forcePlotUpdate();
}

void SVDAnalysisDialog::onCmbPlaneChanged(int index)
{
    //get the current selected factor
    QModelIndex modelIndex = ui->svdFactorTreeView->currentIndex();
    if( ! modelIndex.isValid() )
        return;
    SVDFactor* factor = static_cast<SVDFactor*>( modelIndex.internalPointer() );

    if( index == 0 )
        factor->setPlaneOrientation( SVDFactorPlaneOrientation::XY );
    if( index == 1 )
        factor->setPlaneOrientation( SVDFactorPlaneOrientation::XZ );
    if( index == 2 )
        factor->setPlaneOrientation( SVDFactorPlaneOrientation::YZ );

    ui->spinSlice->setMaximum( factor->getCurrentPlaneNumberOfSlices()-1 ); //1st == 0; last == total-1

    onFactorClicked( modelIndex );

    forcePlotUpdate();
}

void SVDAnalysisDialog::onSpinSliceChanged(int value)
{
    //get the current selected factor
    QModelIndex modelIndex = ui->svdFactorTreeView->currentIndex();
    if( ! modelIndex.isValid() )
        return;
    SVDFactor* factor = static_cast<SVDFactor*>( modelIndex.internalPointer() );

    factor->setCurrentSlice( value );

    forcePlotUpdate();
}

void SVDAnalysisDialog::onSave()
{
    spectral::array *sum = m_tree->getSumOfSelectedFactors();
    emit sumOfFactorsComputed( sum );
}
