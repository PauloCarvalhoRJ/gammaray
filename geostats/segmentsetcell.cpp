#include "segmentsetcell.h"

double SegmentSetCell::readValueFromDataSet() const
{
    return m_segmentSet->data( m_sampleIndex, _dataIndex );
}
