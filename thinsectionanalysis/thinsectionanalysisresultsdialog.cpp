#include "thinsectionanalysisresultsdialog.h"
#include "ui_thinsectionanalysisresultsdialog.h"

#include "widgets/barchartwidget.h"
#include "widgets/imageviewerwidget.h"
#include "thinsectionanalysis/thinsectionanalysiscluster.h"
#include "thinsectionanalysis/thinsectionanalysistablemodel.h"

ThinSectionAnalysisResultsDialog::ThinSectionAnalysisResultsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ThinSectionAnalysisResultsDialog)
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

    //adds the bars to the chart, assiging a percentage of the total.
    for( const ThinSectionAnalysisClusterPtr cluster : clusterSet->getClusters() )
        m_barChartWidget->addBar( cluster->getName(), cluster->getProportion() * 100.0, cluster->getColor() );

    //adjusts the max value for the Y axis.
    m_barChartWidget->setMaxY( m_clusterSet->getMaxProportion() * 100.0 );

    ThinSectionAnalysisTableModel* tableModel = new ThinSectionAnalysisTableModel();
    tableModel->setClusters( clusterSet );

    ui->tableView->setModel( tableModel );
}

void ThinSectionAnalysisResultsDialog::setOutputImage(const QString path)
{
    m_imageViewerWidget->loadFile( path );
}
