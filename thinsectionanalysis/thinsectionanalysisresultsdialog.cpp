#include "thinsectionanalysisresultsdialog.h"
#include "ui_thinsectionanalysisresultsdialog.h"

#include "widgets/barchartwidget.h"
#include "widgets/imageviewerwidget.h"
#include "thinsectionanalysis/thinsectionanalysiscluster.h"

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
    ui->frmRightPane->layout()->addWidget( m_barChartWidget );

    //adds the image viewer widget to the left pane of the dialog.
    m_imageViewerWidget = new ImageViewerWidget();
    ui->frmLeftPane->layout()->addWidget( m_imageViewerWidget );
}

ThinSectionAnalysisResultsDialog::~ThinSectionAnalysisResultsDialog()
{
    delete ui;
}

void ThinSectionAnalysisResultsDialog::setClusters( const std::vector<ThinSectionAnalysisClusterPtr>& clusters )
{
    //compute total number of elements in all clusters
    int total = 0;
    for( const ThinSectionAnalysisClusterPtr& cluster : clusters )
        total += cluster->pixelIndexes.size();
    //adds the bars to the chart, assging a percentage of the total.
    double maxPercent = 0.0;
    for( const ThinSectionAnalysisClusterPtr& cluster : clusters ) {
        double percent = cluster->pixelIndexes.size() / static_cast<double>( total ) * 100.0;
        maxPercent = std::max( maxPercent, percent );
        m_barChartWidget->addBar( cluster->getName(), percent, cluster->getColor() );
    }
    //adjusts the max value for the Y axis.
    m_barChartWidget->setMaxY( maxPercent );
}

void ThinSectionAnalysisResultsDialog::setOutputImage(const QString path)
{
    m_imageViewerWidget->loadFile( path );
}
