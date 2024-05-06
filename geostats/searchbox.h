#ifndef SEARCHBOX_H
#define SEARCHBOX_H

#include "geometry/boundingbox.h"
#include "geostats/searchneighborhood.h"

/**
 * The SearchBox class represents a search neighborhood with the shape of a 3D box.
 */
class SearchBox : public SearchNeighborhood
{
public:

    SearchBox( const BoundingBox& boundingBox );

    /** Move constructor. */
    SearchBox( SearchBox&& right_hand_side );


    ////-------------SearchNeighborhood interface------------------------------------------
    virtual void getBBox( double centerX, double centerY, double centerZ,
                          double& minX, double& minY, double& minZ,
                          double& maxX, double& maxY, double& maxZ ) const;

    virtual bool isInside(double centerX, double centerY, double centerZ,
                          double x, double y, double z ) const;

    virtual bool hasSpatialFiltering() const { return false; }

    virtual void performSpatialFilter( double centerX, double centerY, double centerZ,
                                       std::vector< IndexedSpatialLocationPtr >& samplesLocations,
                                       const SearchStrategy& parentSearchStrategy ) const {}
    ////-------------------------------------------------------------------------------------


    BoundingBox m_boundingBox;
};

#endif // SEARCHBOX_H
