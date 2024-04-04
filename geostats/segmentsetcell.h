#ifndef SEGMENTSETCELL_H
#define SEGMENTSETCELL_H

#include "datacell.h"
#include "domain/segmentset.h"
#include <qglobal.h> //uint

class SegmentSetCell : public DataCell
{
public:

    /** Preferable constructor.
     * @param pointSet The point set this cell refers to.
     * @param dataIndex Data column index of the point set used to get values from (starts with 0) as the point sets are multivalued.
     * @param sampleIndex Data row index of the point set.
     */
    inline SegmentSetCell( SegmentSet* segmentSet, int dataIndex, int sampleIndex )  : DataCell( dataIndex, segmentSet ),
        m_segmentSet(segmentSet), m_sampleIndex( sampleIndex )
    {
        _center._x = ( segmentSet->data( sampleIndex, segmentSet->getXindex()-1 ) +
                       segmentSet->data( sampleIndex, segmentSet->getXFinalIndex()-1 ) ) / 2.0 ;
        _center._y = ( segmentSet->data( sampleIndex, segmentSet->getYindex()-1 ) +
                       segmentSet->data( sampleIndex, segmentSet->getYFinalIndex()-1 ) ) / 2.0;
        _center._z = 0.0;
        if( segmentSet->is3D() )
            _center._z = ( segmentSet->data( sampleIndex, segmentSet->getZindex()-1 ) +
                           segmentSet->data( sampleIndex, segmentSet->getZFinalIndex()-1 ) ) / 2.0;
    }

    // DataCell	interface
    virtual double readValueFromDataSet() const;
    virtual double readValueFromDataSet( uint dataColumnIndex ) const;
    virtual uint64_t getDataRowIndex() const { return m_sampleIndex; }

    SegmentSet* m_segmentSet;
    uint m_sampleIndex;


};

#endif // SEGMENTSETCELL_H
