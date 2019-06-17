#ifndef DATACELL_H
#define DATACELL_H

#include "spatiallocation.h"
#include <cmath>
#include <memory>
#include <iostream>
#include <set>

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
	inline double computeCartesianDistance( const DataCell &fromCell ){
		double dx = std::abs( _center._x - fromCell._center._x );
		double dy = std::abs( _center._y - fromCell._center._y );
		double dz = std::abs( _center._z - fromCell._center._z );
		_cartesianDistance = std::sqrt( dx*dx + dy*dy + dz*dz );
		return _cartesianDistance;
	}

	/** The distance computed in computeCartesianDistance(). */
	double _cartesianDistance;

	/** Returns the data value this cell refers to.
	 */
	virtual double readValueFromDataSet() const = 0;

    /** Returns the data value this cell refers to.
     * It accepts a column index of any variable the data set this data cell
     * refers to.
     */
    virtual double readValueFromDataSet( unsigned int dataColumnIndex ) const = 0;

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

typedef std::shared_ptr<DataCell> DataCellPtr;

/** The DataCellPtr comparator class. */
struct DataCellPtrComparator
{
	bool operator()(const DataCellPtr &d1, const DataCellPtr &d2) const {
		return d1->_cartesianDistance > d2->_cartesianDistance;
	}
};

/** An ordered container by the DataCell's Cartesian distance. */
typedef std::multiset<DataCellPtr, DataCellPtrComparator> DataCellPtrMultiset;


#endif // DATACELL_H
