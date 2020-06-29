#include "thinsectionanalysisresultsdialog.h"
#include "ui_thinsectionanalysisresultsdialog.h"

#include "widgets/barchartwidget.h"
#include "widgets/imageviewerwidget.h"
#include "thinsectionanalysis/thinsectionanalysiscluster.h"
#include "thinsectionanalysis/thinsectionanalysistablemodel.h"

#include <iostream>

ThinSectionAnalysisResultsDialog::ThinSectionAnalysisResultsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ThinSectionAnalysisResultsDialog),
    m_tableModel( nullptr )
{
    ui->setupUi(this);

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    this->setWindowTitle( "Thin Section Analysis Results" );

    //adds the histogram widget to the right pane of the dialog.
    m_barChartWidget = new BarChartWidget();
    m_barChartWidget->setChartTitle( "Distribution of clusters" );
    m_barChartWidget->setMaxY( 100.0 );
    m_barChartWidget->setAxisYLabel( "%" );
    m_barChartWidget->setAxisXLabel( "clusters" );
    ui->tabHistogram->layout()->addWidget( m_barChartWidget );

    //adds the image viewer widget to the left pane of the dialog.
    m_imageViewerWidget = new ImageViewerWidget();
    ui->frmLeftPane->layout()->addWidget( m_imageViewerWidget );

    reconfigureUI();
}

ThinSectionAnalysisResultsDialog::~ThinSectionAnalysisResultsDialog()
{
    delete ui;
}

void ThinSectionAnalysisResultsDialog::setClusters( ThinSectionAnalysisClusterSetPtr clusterSet )
{
    //sets the member pointer.
    m_clusterSet = clusterSet;

    //compute the proportion of each cluster
    clusterSet->computeClustersProportions();

    //updates the bar chart display.
    updateBarChart();

    //(re)create a table model from the clusters
    if( m_tableModel )
        delete m_tableModel;
    m_tableModel = new ThinSectionAnalysisTableModel();
    m_tableModel->setClusters( clusterSet );
    connect( m_tableModel, SIGNAL(dataEdited()), this, SLOT(onDataEdited()));

    //assign the clusters-based table model to the table view.
    ui->tableView->setModel( m_tableModel );

    //Once there is a data model, there is a valid selection model.
    // selection changes in the categories table shall trigger a slot.
    // TODO: verify whether the previous selection model is being deleted along the table model.
    //       otherwiser, multiple connections may be taking place.
    QItemSelectionModel *selectionModel = ui->tableView->selectionModel();
    connect(selectionModel, SIGNAL( selectionChanged(const QItemSelection&, const QItemSelection&) ),
            this,           SLOT( onSelectionChangedInTable(const QItemSelection&, const QItemSelection&) ));

}

void ThinSectionAnalysisResultsDialog::setOutputImage(const QString path)
{
    m_imageViewerWidget->loadFile( path );
}

void ThinSectionAnalysisResultsDialog::updateBarChart()
{
    m_barChartWidget->clearBars();

    //adds the bars to the chart, assiging a percentage of the total.
    for( const ThinSectionAnalysisClusterPtr& cluster : m_clusterSet->getClusters() )
        m_barChartWidget->addBar( cluster->getName(), cluster->getProportion() * 100.0, cluster->getColor() );

    //adjusts the max value for the Y axis.
    m_barChartWidget->setMaxY( m_clusterSet->getMaxProportion() * 100.0 );
}

void ThinSectionAnalysisResultsDialog::onDataEdited()
{
    updateBarChart();
}

void ThinSectionAnalysisResultsDialog::onJoinCategories()
{
    //Merge the clusters.
    m_clusterSet->mergeClusters( m_selectedClusterIndexes );

    //Update the bar chart view.
    updateBarChart();

    //Update the table view.
    //forces the view to update by rereading the table model
    //TODO: not very elegant, but Qt still seems to lack a better notification mechanism
    //      that we can call from outside the data model (the dataChanged() signal
    //      is not callable from outside).
    ui->tableView->viewport()->update();

    //Update the image view.
    m_imageViewerWidget->loadFile( m_clusterSet->getSegmentedImagePath() );
}

void ThinSectionAnalysisResultsDialog::onSelectionChangedInTable( const QItemSelection& /*newSelection*/,
                                                                  const QItemSelection& /*oldSelection*/ )
{
    const QModelIndexList selectedIndexes = ui->tableView->selectionModel()->selectedIndexes();

    //repopulate the container of selected indexes
    m_selectedClusterIndexes.clear();
    for( const QModelIndex& index : selectedIndexes ){
        int rowIndex = index.row();
        m_selectedClusterIndexes.push_back( rowIndex );
    }

    reconfigureUI();
}

void ThinSectionAnalysisResultsDialog::reconfigureUI()
{
    if( m_selectedClusterIndexes.size() >= 2 )
        ui->btnJoinCategories->setEnabled( true );
    else
        ui->btnJoinCategories->setEnabled( false );
}
