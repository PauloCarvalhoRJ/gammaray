#ifndef SPATIALINDEX_H
#define SPATIALINDEX_H

#include <QList>

class PointSet;
class CartesianGrid;
class DataCell;
class SearchStrategy;

/**
 * This class exposes functionalities related to spatial indexes and queries with GammaRay objects.
 */
class SpatialIndexPoints
{
public:
    SpatialIndexPoints();
	virtual ~SpatialIndexPoints();

    /** Fills the index with the PointSet points (bulk load).
     * It erases any previously indexed points.
     * @param tolerance Sets the size of the bouding boxes around each point.
     */
    static void fill( PointSet* ps, double tolerance );

	/** Fills the index with the CartesianGrid cells (bulk load).
	 * It erases any previously indexed points.
	 */
	static void fill( CartesianGrid* cg );

	/**
     * Returns the indexes of the n-nearest points to the point given by its index.
	 * The indexes are the data record indexes (file data lines) of the DataFile used to fill
     * the index.
     */
    static QList<uint> getNearest( uint index, uint n );

    /**
	 * Returns the data line indexes of the n-nearest points within the given distance
     * to the point given by its index. The indexes are the point indexes
     * (file data lines) of the PointSet used fill the index.  May return
     * an empty list.
     * @param distance The distance the returned points must be within.
     */
    static QList<uint> getNearestWithin(uint index, uint n, double distance);

	/**
	 * Returns the data line indexes of the n-nearest points within the given neighborhood
	 * centered at given data cell (e.g. grid cell). The indexes are the point indexes
	 * (file data lines) of the PointSet used fill the index.  May return
	 * an empty list.
	 */
	static QList<uint> getNearestWithin(const DataCell& dataCell,
										const SearchStrategy & searchStrategy );


    /** Clears the spatial index. */
    static void clear();

};

#endif // SPATIALINDEX_H
