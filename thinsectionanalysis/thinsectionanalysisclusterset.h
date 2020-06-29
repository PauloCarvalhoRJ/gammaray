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

    /** Merges the clusters given a list of indexes. The first cluster in the vector receives the
     * pixel indexes of the others and remains in this set. If the passed vector has less than two
     * elements, nothing happens.  The segmented image is also repainted accordingly (if it has been
     * set and exists).
     */
    void mergeClusters( const std::vector<int>& indexes );

    /** The segmented image path refers to the file that contains the pixels
     * painted with the colors of the clusters.  This image is normally created
     * after a clustering algorithm completes.
     */
    QString getSegmentedImagePath() const;
    void setSegmentedImagePath(const QString &segmentedImagePath);

private:
    std::vector< ThinSectionAnalysisClusterPtr > m_clusters;
    double m_maxProportion;
    QString m_segmentedImagePath;

    /** Repaints the pixels of the image referenced by m_segmentedImagePath with the current
     * cluster data.
     */
    void updateSegmentedImage();
};
typedef std::shared_ptr< ThinSectionAnalysisClusterSet > ThinSectionAnalysisClusterSetPtr;



#endif // THINSECTIONANALYSISCLUSTERSET_H
