#ifndef DATACELL_H
#define DATACELL_H

#include "spatiallocation.h"

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

#endif // DATACELL_H
