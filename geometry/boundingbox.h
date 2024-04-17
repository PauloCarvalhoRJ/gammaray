#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H


/**
 * The BoundingBox class represents the spatial extent of some geometric object.
 */
class BoundingBox
{
public:
    BoundingBox();

    BoundingBox(double minX, double minY, double minZ,
                double maxX, double maxY, double maxZ);

    double getXsize() const { return m_maxX - m_minX; }
    double getYsize() const { return m_maxY - m_minY; }
    double getZsize() const { return m_maxZ - m_minZ; }

    double getCenterX() const { return (m_maxX+m_minX)/2; }
    double getCenterY() const { return (m_maxY+m_minY)/2; }
    double getCenterZ() const { return (m_maxZ+m_minZ)/2; }

    double m_minX, m_minY, m_minZ, m_maxX, m_maxY, m_maxZ;
};

#endif // BOUNDINGBOX_H
