#ifndef THINSECTIONANALYSISCLUSTERSET_H
#define THINSECTIONANALYSISCLUSTERSET_H

#include "thinsectionanalysis/thinsectionanalysiscluster.h"

/** This class contains a collection of ThinSectionAnalysisCluster objects. */
class ThinSectionAnalysisClusterSet
{
public:
    ThinSectionAnalysisClusterSet();

    /** Returns the collection of clusters in this set. */
    std::vector< ThinSectionAnalysisClusterPtr >& getClusters() { return m_clusters; }

    /** Returns the number of clusters in this set. */
    int getClusterCount() const { return m_clusters.size(); }

    /** Returns whether this set has clusters. */
    int isEmpty() const { return m_clusters.empty(); }

    /** Returns one cluster given its index. */
    ThinSectionAnalysisClusterPtr getCluster( int index ) const { return m_clusters[index]; }

    /** Pre-allocates memory for cluster storage.  This is useful to have a faster code in case
     * of a great number of clusters is expected.
     */
    void reserve( int numberOfClusters ){ m_clusters.reserve( numberOfClusters ); }

    /** Adds a cluster to this set. */
    void addCluster( ThinSectionAnalysisClusterPtr cluster ){ m_clusters.push_back( cluster ); }

    /** Removes all clusters whose toDelete members are set to true. */
    void purgeClustersMarkedForDeletion();

    /** Returns the total number of pixels/elements in all clusters. */
    int getTotalPixelCount();

    /** Computes and sets the m_proportion member of each of the clusters in this set.
     * This method also updates the m_maxProportion member of this class.
     */
    int computeClustersProportions();

    /** Returns the value in m_maxProportion member.  It is updated in computeClustersProportions() method.
     * This is the value that correspond to the greatest proportion found amongst the clusters.
     */
    double getMaxProportion() const;

private:
    std::vector< ThinSectionAnalysisClusterPtr > m_clusters;
    double m_maxProportion;
};
typedef std::shared_ptr< ThinSectionAnalysisClusterSet > ThinSectionAnalysisClusterSetPtr;



#endif // THINSECTIONANALYSISCLUSTERSET_H
