#include "searchannulus.h"

#include <limits>

SearchAnnulus::SearchAnnulus(double innerRadius, double outerRadius,
                             uint minNumberOfSamples, uint maxNumberOfSamples) :
    m_innerRadius(innerRadius),
    m_outerRadius(outerRadius),
    m_maxNumberOfSamples(maxNumberOfSamples),
    m_minNumberOfSamples(minNumberOfSamples)
{}

void SearchAnnulus::getBBox(double centerX, double centerY, double centerZ,
                            double &minX, double &minY, double &minZ,
                            double &maxX, double &maxY, double &maxZ) const
{
    maxX = centerX + m_outerRadius;
    minX = centerX - m_outerRadius;
    maxY = centerY + m_outerRadius;
    minY = centerY - m_outerRadius;
    maxZ =  std::numeric_limits<double>::max();
    minZ = -std::numeric_limits<double>::max();
}

bool SearchAnnulus::isInside(double centerX, double centerY, double centerZ,
                             double x, double y, double z) const
{
    double localX = x - centerX;
    double localY = y - centerY;
    double outer_dx = localX/m_outerRadius;
    double outer_dy = localY/m_outerRadius;
    double inner_dx = localX/m_innerRadius;
    double inner_dy = localY/m_innerRadius;
    return ( outer_dx*outer_dx + outer_dy*outer_dy <= 1.0 ) &&
           ( inner_dx*inner_dx + inner_dy*inner_dy  > 1.0 );
}
