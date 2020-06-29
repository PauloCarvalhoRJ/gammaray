#include "thinsectionanalysisclusterset.h"

#include <QApplication>
#include <QImage>
#include <QProgressDialog>

ThinSectionAnalysisClusterSet::ThinSectionAnalysisClusterSet()
{

}

void ThinSectionAnalysisClusterSet::purgeClustersMarkedForDeletion()
{
    m_clusters.erase(
        std::remove_if(
            m_clusters.begin(),
            m_clusters.end(),
            [](ThinSectionAnalysisClusterPtr cluster) -> bool {
                return cluster->toDelete;
            }
        ),
            m_clusters.end() );
}

int ThinSectionAnalysisClusterSet::getTotalPixelCount()
{
    //compute total number of elements in all clusters
    int total = 0;
    for( const ThinSectionAnalysisClusterPtr cluster : getClusters() )
        total += cluster->pixelIndexes.size();
    return total;
}

int ThinSectionAnalysisClusterSet::computeClustersProportions()
{
    //compute total number of elements in all clusters
    int total = getTotalPixelCount();

    //adds the bars to the chart, assiging a percentage of the total.
    m_maxProportion = 0.0;
    for( ThinSectionAnalysisClusterPtr cluster : getClusters() ) {
        double proportion = cluster->pixelIndexes.size() / static_cast<double>( total );
        cluster->setProportion( proportion );
        m_maxProportion = std::max( m_maxProportion, proportion );
    }

    return total;
}

double ThinSectionAnalysisClusterSet::getMaxProportion() const
{
    return m_maxProportion;
}

void ThinSectionAnalysisClusterSet::mergeClusters(const std::vector<int> &indexes)
{
    if( indexes.size() < 2 )
        return;

    //get pointer to the first cluster.  This is the one that'll remain.
    ThinSectionAnalysisClusterPtr firstCluster = getCluster( indexes[0] );

    //for each of the cluster indexes from the 2nd and on.
    for( int i = 1;  i < indexes.size(); ++i ){

        //get the cluster index
        int iCluster = indexes[i];

        //get the cluster
        ThinSectionAnalysisClusterPtr otherCluster = getCluster( iCluster );

        //merge it into the first cluster
        firstCluster->merge(*otherCluster );

        //mark it for deletion
        otherCluster->toDelete = true;

    }

    //remove the clusters marked for deletion
    purgeClustersMarkedForDeletion();

    //update the segmented image (if it exists).
    updateSegmentedImage();

}

QString ThinSectionAnalysisClusterSet::getSegmentedImagePath() const
{
    return m_segmentedImagePath;
}

void ThinSectionAnalysisClusterSet::setSegmentedImagePath(const QString &segmentedImagePath)
{
    m_segmentedImagePath = segmentedImagePath;
}

void ThinSectionAnalysisClusterSet::updateSegmentedImage()
{
    //loading the segmented image as string of 4-byte tuples:
    //Alpha, Red, Green and Blue as values between 0 and 255.
    QImage segmentedImage( m_segmentedImagePath );
    segmentedImage = segmentedImage.convertToFormat( QImage::Format_ARGB32 );

    //get image info.
    int nCols = segmentedImage.size().width();
    int nRows = segmentedImage.size().height();
    int pixelCount = nCols * nRows;
    const int bytesPerPixel = 4;

    uchar r, g, b;
    uint clusterCounter = 0;

    QProgressDialog progressDialog;
    progressDialog.show();
    progressDialog.setLabelText("Repainting segmented image...");
    progressDialog.setMinimum(0);
    progressDialog.setMaximum( getClusterCount() );
    progressDialog.setValue(0);

    //for each cluster
    int sequential = 0;
    for( ThinSectionAnalysisClusterPtr cluster : getClusters()  ){
        //assign a constrasting color to the cluster's pixels
        //GSLib's colors form a good sequence of constrasting colors
        QColor clusterColor = cluster->getColor();
        r = clusterColor.red();
        g = clusterColor.green();
        b = clusterColor.blue();
        //for each pixel assigned to the cluster
        for( uint iPixel : cluster->pixelIndexes ){
            //convert its index to a lin x col coordinate
            uint line = iPixel / nCols;
            uint column = iPixel % nCols;
            //set the pixel color
            segmentedImage.setPixel( column, line, qRgb( r, g, b ) );
        }
        //give a name to the cluster
        cluster->setName( QString::number( ++sequential ) );
        //assign the color to the cluster
        cluster->setColor( clusterColor );
        //update the progress bar
        progressDialog.setValue(clusterCounter);
        QApplication::processEvents();
    }
    progressDialog.setLabelText("Saving segmented image...");
    segmentedImage.save( m_segmentedImagePath );
}
