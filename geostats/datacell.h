#ifndef DATACELL_H
#define DATACELL_H

#include "spatiallocation.h"
#include <cmath>

class DataFile;

/** Data structure containing information of a data set cell (point of a point cloud or cell of a grid). */
class DataCell
{

public:
	//--------------------public member variables---------------------

	/** Spatial coordinates of the cell center. */
	SpatialLocation _center;

	/** Data index in multi-valued cells. */
	int _dataIndex;

	/** The data file that this cell referes to. */
	DataFile* _dataFile;

	/** Computes the Cartesian distance from the given cell.
	 * The result is also stored in _cartesianDistance member variable.
	 */
	inline double computeCartesianDistance( DataCell &fromCell ){
		double dx = std::abs( _center._x - fromCell._center._x );
		double dy = std::abs( _center._y - fromCell._center._y );
		double dz = std::abs( _center._z - fromCell._center._z );
		_cartesianDistance = std::sqrt( dx*dx + dy*dy + dz*dz );
		return _cartesianDistance;
	}

	/** The distance computed in computeCartesianDistance(). */
	double _cartesianDistance;

protected:
	inline DataCell( int dataIndex ) :
		_dataIndex( dataIndex ),
		_dataFile( nullptr )
	{}
	inline DataCell( int dataIndex, DataFile* dataFile ) :
		_dataIndex( dataIndex ),
		_dataFile( dataFile )
	{}
};

/**
 * This global non-member less-than operator enables the DataCell class as key-able
 * in STL or STL-like ordered containers.
 */
inline bool operator<(const DataCell &d1, const DataCell &d2){
	return d1._cartesianDistance < d2._cartesianDistance;
}


#endif // DATACELL_H
