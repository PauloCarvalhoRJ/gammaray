#include "searchsphericalshell.h"

SearchSphericalShell::SearchSphericalShell(double innerRadius, double outerRadius) :
    SearchAnnulus( innerRadius, outerRadius )
{}

void SearchSphericalShell::getBBox(double centerX, double centerY, double centerZ,
                                 double &minX, double &minY, double &minZ,
                                 double &maxX, double &maxY, double &maxZ) const
{
    maxX = centerX + m_outerRadius;
    minX = centerX - m_outerRadius;
    maxY = centerY + m_outerRadius;
    minY = centerY - m_outerRadius;
    maxZ = centerZ + m_outerRadius;
    minZ = centerZ - m_outerRadius;
}

bool SearchSphericalShell::isInside(double centerX, double centerY, double centerZ,
                                  double x, double y, double z) const
{
    double localX = x - centerX;
    double localY = y - centerY;
    double localZ = z - centerZ;
    double outer_dx = localX/m_outerRadius;
    double outer_dy = localY/m_outerRadius;
    double outer_dz = localZ/m_outerRadius;
    double inner_dx = localX/m_innerRadius;
    double inner_dy = localY/m_innerRadius;
    double inner_dz = localZ/m_innerRadius;
    return ( outer_dx*outer_dx + outer_dy*outer_dy + outer_dz*outer_dz <= 1.0 ) &&
           ( inner_dx*inner_dx + inner_dy*inner_dy + inner_dz*inner_dz > 1.0 );
}
