#ifndef SPATIALINDEX_H
#define SPATIALINDEX_H

#include <QList>

class PointSet;

/**
 * This class exposes functionalities related to spatial indexes and queries with GammaRay objects.
 */
class SpatialIndexPoints
{
public:
    SpatialIndexPoints();

    /** Fills the index with the PointSet points (bulk load).
     * It erases any previously indexed points.
     * @param tolerance Sets the size of the bouding boxes around each point.
     */
    static void fill( PointSet* ps, double tolerance );

    /**
     * Returns the indexes of the n-nearest points to the point given by its index.
     * The indexes are the point indexes (file data lines) of the PointSet used fill
     * the index.
     */
    static QList<uint> getNearest( uint index, uint n );

    /**
     * Returns the indexes of the n-nearest points within the diven distance
     * to the point given by its index. The indexes are the point indexes
     * (file data lines) of the PointSet used fill the index.  May return
     * an empty list.
     * @param distance The distance the returned points must be within.
     */
    static QList<uint> getNearestWithin(uint index, uint n, double distance);

    /** Clears the spatial index. */
    static void clear();

};

#endif // SPATIALINDEX_H
