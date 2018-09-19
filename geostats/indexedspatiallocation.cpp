#include "indexedspatiallocation.h"

IndexedSpatialLocation::IndexedSpatialLocation() :
	SpatialLocation(),
	_index( 0 )
{
}

IndexedSpatialLocation::IndexedSpatialLocation(double x, double y, double z, uint index) :
	SpatialLocation( x, y, z ),
	_index( index )
{
}
