#ifndef SEARCHNEIGHBORHOOD_H
#define SEARCHNEIGHBORHOOD_H

#include <memory>
#include <vector>
#include "spatiallocation.h"

/** This class represents a generic search neighborhood. */
class SearchNeighborhood
{
public:
	SearchNeighborhood();

	/** Computes the bounding box of this neighborhood when centered at the given location. */
	virtual void getBBox( double centerX, double centerY, double centerZ,
						  double& minX, double& minY, double& minZ,
                          double& maxX, double& maxY, double& maxZ ) const = 0;

	/** Tests whether the given point is inside the neighborhood centered at (centerX, centerY, centerZ). */
	virtual bool isInside(double centerX, double centerY, double centerZ,
						  double x, double y, double z ) const = 0;

    /** Returns whether the search neighborhood has some additional spatial filtering
     * other than the simple n-nearest points (e.g. octant/sector search).  That is,
     * whether the implementation has something to do in performSpatialFilter() method.
     */
    virtual bool hasSpatialFiltering() const = 0;

    /**
     * Spatially filters the samples locations in the passed vector (e.g. octant/sector search).
     * The result are the locations that remain in the passed vector itself.
     */
    virtual void performSpatialFilter( std::vector< SpatialLocationPtr >& samplesLocations ) const = 0;
};

typedef std::shared_ptr<SearchNeighborhood> SearchNeighborhoodPtr;

#endif // SEARCHNEIGHBORHOOD_H
