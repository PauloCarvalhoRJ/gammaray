#ifndef POINTSETCELL_H
#define POINTSETCELL_H

#include "datacell.h"
#include "domain/pointset.h"
#include <qglobal.h> //uint

/** Data structure containing information of a point set data sample (point). */
class PointSetCell : public DataCell
{
public:

	/** Preferable constructor.
	 * @param pointSet The point set this cell refers to.
	 * @param dataIndex Data column index of the point set used to get values from (starts with 0) as the point sets are multivalued.
	 * @param sampleIndex Data row index of the point set.
	 */
	inline PointSetCell( PointSet* pointSet, int dataIndex, int sampleIndex )  : DataCell( dataIndex, pointSet ),
		m_pointSet(pointSet), m_sampleIndex( sampleIndex )
	{
		_center._x = pointSet->data( sampleIndex, pointSet->getXindex()-1 );
		_center._y = pointSet->data( sampleIndex, pointSet->getYindex()-1 );
		_center._z = 0.0;
		if( pointSet->is3D() )
			_center._z = pointSet->data( sampleIndex, pointSet->getZindex()-1 );
	}

	// DataCell	interface
	virtual double readValueFromDataSet() const;
    virtual double readValueFromDataSet( uint dataColumnIndex ) const;

	PointSet* m_pointSet;
	uint m_sampleIndex;

};

#endif // POINTSETCELL_H
