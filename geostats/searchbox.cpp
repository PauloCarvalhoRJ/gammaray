#include "searchbox.h"

SearchBox::SearchBox( const BoundingBox& boundingBox ) :
    m_boundingBox( boundingBox )
{

}

void SearchBox::getBBox(double centerX, double centerY, double centerZ,
                        double &minX, double &minY, double &minZ,
                        double &maxX, double &maxY, double &maxZ) const
{
    double semiWidth  = m_boundingBox.getXsize() / 2;
    double semiLength = m_boundingBox.getYsize() / 2;
    double semiHeight = m_boundingBox.getZsize() / 2;
    minX = centerX - semiWidth;
    maxX = centerX + semiWidth;
    minY = centerY - semiLength;
    maxY = centerY + semiLength;
    minZ = centerZ - semiHeight;
    maxZ = centerZ + semiHeight;
}

bool SearchBox::isInside(double centerX, double centerY, double centerZ,
                         double x, double y, double z) const
{
    double minX, minY, minZ, maxX, maxY, maxZ;
    //the geometry for inside test of a search box is exactly its bounding box
    getBBox( centerX, centerY, centerZ, minX, minY, minZ, maxX, maxY, maxZ );
    return ( x >= minX && x <= maxX && y >= minY && y <= maxY && z >= minZ && z <= maxZ );
}
