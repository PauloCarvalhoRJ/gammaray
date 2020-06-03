#ifndef THINSECTIONANALYSISCLUSTER_H
#define THINSECTIONANALYSISCLUSTER_H

#include <vector>
#include <array>
#include <memory>

class ThinSectionAnalysisCluster{

public:

    /** The constructor must accept the reference to the vector of the feature vectors
     *  of all pixels to reuse it across all clusters.
     */
    ThinSectionAnalysisCluster( const std::vector< std::array< float, 12 > >& pRefFeatureVectors );

    /** Adds a pixel index to this cluster and updates its center. */
    void addPixelIndex( unsigned int pixelIndex );

    /** Computes the similarity of two 12-element vectors according to
     * Izadi et al (2020): "Altered mineral segmentation using incremental-dynamic clustering", eq 2.
     * the result varies between 0.0 (least similar) and 1.0 (most similar).
     */
    float similarity(const std::array< float, 12 >& v1, const std::array< float, 12 >& v2) const;

    /** Returns whether the given pixel is similar to this cluster. */
    bool isSimilar( unsigned int pixelIndex, float sigma ) const;

    /** Adds the pixel indexes of another cluster to this cluster's pixelIndexes list
     * and updates the center.
     */
    void merge( const ThinSectionAnalysisCluster& otherCluster );

    /** Reference to the vector of feature vectors of all pixels. */
    const std::vector< std::array< float, 12 > >& refFeatureVectors;

    /** The cluster only stores pixel indexes, not the feature vectors to save memory
     * theses indexes refer to feature vectors in refFeatureVectors.
     */
    std::vector<unsigned int> pixelIndexes;

    /** The average feature vector of all pixels indexed by this cluster. */
    std::array< float, 12 > center;

    /** Flag used to mark a cluster for deletion. */
    bool toDelete;
};

typedef std::shared_ptr< ThinSectionAnalysisCluster > ThinSectionAnalysisClusterPtr;

#endif // THINSECTIONANALYSISCLUSTER_H
