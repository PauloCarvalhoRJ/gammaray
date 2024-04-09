#include "searchverticaldumbbell.h"

SearchVerticalDumbbell::SearchVerticalDumbbell(double height_of_each_cylinder,
                                               double separation_between_the_cylinders,
                                               double radius) :
    m_height_of_each_cylinder( height_of_each_cylinder ),
    m_separation_between_the_cylinders(separation_between_the_cylinders),
    m_radius( radius )
{}

void SearchVerticalDumbbell::getBBox(double centerX, double centerY, double centerZ,
                            double &minX, double &minY, double &minZ,
                            double &maxX, double &maxY, double &maxZ) const
{
    maxX = centerX + m_radius;
    minX = centerX - m_radius;
    maxY = centerY + m_radius;
    minY = centerY - m_radius;
    maxZ = centerZ + m_separation_between_the_cylinders/2 + m_height_of_each_cylinder;
    minZ = centerZ - m_separation_between_the_cylinders/2 - m_height_of_each_cylinder;
}

bool SearchVerticalDumbbell::isInside(double centerX, double centerY, double centerZ,
                             double x, double y, double z) const
{
    double localX = x - centerX;
    double localY = y - centerY;
    double localZ = z - centerZ;
    double dx = localX/m_radius;
    double dy = localY/m_radius;
    return ( dx*dx + dy*dy <= 1.0 ) &&
           ( localZ >= (-m_height_of_each_cylinder - m_separation_between_the_cylinders/2) &&
             localZ <= ( m_height_of_each_cylinder + m_separation_between_the_cylinders/2) );
}
