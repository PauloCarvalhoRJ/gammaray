#include "thinsectionanalysisclusterset.h"

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
