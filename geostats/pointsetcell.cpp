#include "pointsetcell.h"

double PointSetCell::readValueFromDataSet() const
{
	return m_pointSet->data( m_sampleIndex, _dataIndex );
}
