#include "searchwasher.h"

SearchWasher::SearchWasher(double innerRadius, double outerRadius, double zThickness) :
    SearchAnnulus( innerRadius, outerRadius ),
    m_zThickness( zThickness )
{}

void SearchWasher::getBBox(double centerX, double centerY, double centerZ,
                            double &minX, double &minY, double &minZ,
                            double &maxX, double &maxY, double &maxZ) const
{
    maxX = centerX + m_outerRadius;
    minX = centerX - m_outerRadius;
    maxY = centerY + m_outerRadius;
    minY = centerY - m_outerRadius;
    maxZ = centerZ + m_zThickness/2;
    minZ = centerZ - m_zThickness/2;
}

bool SearchWasher::isInside(double centerX, double centerY, double centerZ,
                             double x, double y, double z) const
{
    double localX = x - centerX;
    double localY = y - centerY;
    double localZ = z - centerZ;
    double outer_dx = localX/m_outerRadius;
    double outer_dy = localY/m_outerRadius;
    double inner_dx = localX/m_innerRadius;
    double inner_dy = localY/m_innerRadius;
    return ( outer_dx*outer_dx + outer_dy*outer_dy <= 1.0 ) &&
           ( inner_dx*inner_dx + inner_dy*inner_dy  > 1.0 ) &&
           ( localZ >= -m_zThickness/2 && localZ <= m_zThickness/2 );
}

