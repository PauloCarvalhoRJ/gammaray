#include "segmentsetcell.h"

double SegmentSetCell::readValueFromDataSet() const
{
    return m_segmentSet->data( m_sampleIndex, _dataIndex );
}

double SegmentSetCell::readValueFromDataSet(uint dataColumnIndex) const
{
    return m_segmentSet->data( m_sampleIndex, dataColumnIndex );
}
