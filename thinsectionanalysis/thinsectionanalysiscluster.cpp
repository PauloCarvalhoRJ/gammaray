#include "thinsectionanalysiscluster.h"

#include <cmath>

ThinSectionAnalysisCluster::ThinSectionAnalysisCluster(
        const std::vector< std::array< float, 12 > >& pRefFeatureVectors
        ) :
    refFeatureVectors( pRefFeatureVectors ),
    center( {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f} ),
    toDelete( false )
{

}

void ThinSectionAnalysisCluster::addPixelIndex( unsigned int pixelIndex ){
    pixelIndexes.push_back( pixelIndex );
    //mean updating: https://math.stackexchange.com/questions/106700/incremental-averageing
    for( unsigned char j = 0; j < 12; ++j )
        center[j] += ( refFeatureVectors[pixelIndex][j] - center[j] ) / pixelIndexes.size();
}

float ThinSectionAnalysisCluster::similarity(const std::array< float, 12 >& v1,
                                             const std::array< float, 12 >& v2) const
{
    return 1 / ( 1 + std::sqrt( (v1[ 0]-v2[ 0])*(v1[ 0]-v2[ 0])+
                                (v1[ 1]-v2[ 1])*(v1[ 1]-v2[ 1])+
                                (v1[ 2]-v2[ 2])*(v1[ 2]-v2[ 2])+
                                (v1[ 3]-v2[ 3])*(v1[ 3]-v2[ 3])+
                                (v1[ 4]-v2[ 4])*(v1[ 4]-v2[ 4])+
                                (v1[ 5]-v2[ 5])*(v1[ 5]-v2[ 5])+
                                (v1[ 6]-v2[ 6])*(v1[ 6]-v2[ 6])+
                                (v1[ 7]-v2[ 7])*(v1[ 7]-v2[ 7])+
                                (v1[ 8]-v2[ 8])*(v1[ 8]-v2[ 8])+
                                (v1[ 9]-v2[ 9])*(v1[ 9]-v2[ 9])+
                                (v1[10]-v2[10])*(v1[10]-v2[10])+
                                (v1[11]-v2[11])*(v1[11]-v2[11]) ) );
}

bool ThinSectionAnalysisCluster::isSimilar( unsigned int pixelIndex, float sigma ) const{
    return similarity( center, refFeatureVectors[pixelIndex] ) >= sigma;
}

void ThinSectionAnalysisCluster::merge( const ThinSectionAnalysisCluster& otherCluster ){
    //update the center (average of two averages)
    float mySum[12]    = {0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
    for( unsigned char j = 0; j < 12; ++j ) {
        //get the sum of this cluster by multiplying the mean by the current count
        mySum[j] = center[j] * pixelIndexes.size();
        //adds the sum of the other cluster, which is calculated the same way
        mySum[j] += otherCluster.center[j] * otherCluster.pixelIndexes.size();
        //the new average is then the grand sum divided by the combined number of elements
        center[j] = mySum[j] / ( pixelIndexes.size() + otherCluster.pixelIndexes.size() );
    }
    //adds the pixel indexes of the other cluster
    pixelIndexes.insert( pixelIndexes.end(),
                         otherCluster.pixelIndexes.begin(),
                         otherCluster.pixelIndexes.end() );
}
