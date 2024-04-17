#include "boundingbox.h"

BoundingBox::BoundingBox()
{}

BoundingBox::BoundingBox(double minX, double minY, double minZ,
                         double maxX, double maxY, double maxZ) :
    m_minX(minX),
    m_minY(minY),
    m_minZ(minZ),
    m_maxX(maxX),
    m_maxY(maxY),
    m_maxZ(maxZ)
{}
