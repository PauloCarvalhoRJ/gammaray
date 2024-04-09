#ifndef SEARCHANNULUS_H
#define SEARCHANNULUS_H

#include "geostats/searchneighborhood.h"

/**
 * The SearchAnnulus class represents a search neighborhood with the shape of an annulus (2D ring).
 * Being 2D, this search neighborhood is recommended for usage with 2D data sets.
 */
class SearchAnnulus : public SearchNeighborhood
{
public:

    SearchAnnulus(double innerRadius, double outerRadius );

    /** Move constructor. */
    SearchAnnulus( SearchAnnulus&& right_hand_side );


    ////-------------SearchNeighborhood interface------------------------------------------
    /**
     * @note As this search neighborhood is 2D, the returned bounding box extends into infinity in Z direction.
     */
    virtual void getBBox( double centerX, double centerY, double centerZ,
                          double& minX, double& minY, double& minZ,
                          double& maxX, double& maxY, double& maxZ ) const;
    /**
     * @note As this search neighborhood is 2D, the z coordinates are ignored in the test.
     */
    virtual bool isInside(double centerX, double centerY, double centerZ,
                          double x, double y, double z ) const;

    virtual bool hasSpatialFiltering() const { return false; }

    virtual void performSpatialFilter( double centerX, double centerY, double centerZ,
                                       std::vector< IndexedSpatialLocationPtr >& samplesLocations,
                                       const SearchStrategy& parentSearchStrategy ) const {}
    ////-------------------------------------------------------------------------------------


    double m_innerRadius, m_outerRadius;
};

#endif // SEARCHANNULUS_H
