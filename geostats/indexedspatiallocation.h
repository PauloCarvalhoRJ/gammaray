#ifndef INDEXEDSPATIALLOCATION_H
#define INDEXEDSPATIALLOCATION_H

#include "spatiallocation.h"
#include <qglobal.h>

/** The same as SpatialLocation, but with an extra field, index, used when the location is
 * can also be localized by an index or id (e.g. of a vector or database).
 */
class IndexedSpatialLocation : public SpatialLocation
{
public:
	/** Initializes the location at ( 0.0, 0.0, 0.0 ) and with index 0. */
	IndexedSpatialLocation();

	/** Initializes the location at the given coordinates and index. */
	IndexedSpatialLocation( double x, double y, double z, uint index );

	uint _index;
};

typedef std::shared_ptr<IndexedSpatialLocation> IndexedSpatialLocationPtr;

#endif // INDEXEDSPATIALLOCATION_H
