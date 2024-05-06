#ifndef SPATIALINDEX_H
#define SPATIALINDEX_H

#include <QList>
#include <vector>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

class PointSet;
class CartesianGrid;
class DataCell;
class SearchStrategy;
class DataFile;
class GeoGrid;
class SegmentSet;
class GridCell;
class BoundingBox;

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
typedef bg::model::point<double, 3, bg::cs::cartesian> Point3D;
typedef bg::model::box<Point3D> Box;
typedef std::pair<Box, size_t> BoxAndDataIndex;
typedef bgi::rtree< BoxAndDataIndex, bgi::rstar<16,5,5,32> > RStarRtree;
typedef std::pair< BoxAndDataIndex, double > BoxAndDataIndexAndDistance;

/**
 * This class exposes functionalities related to spatial indexes and queries with GammaRay objects.
 */
class SpatialIndex
{
public:
    SpatialIndex();
    virtual ~SpatialIndex();

    /** Fills the index with the PointSet points (bulk load).
     * It erases current index.
     * @param tolerance Sets the size of the bounding boxes around each point.
     */
	void fill( PointSet* ps, double tolerance );

	/** Fills the index with the CartesianGrid cells (bulk load).
     * It erases current index.
     */
	void fill( CartesianGrid* cg );

    /** Fills the index with the GeoGrid cell bounding boxes (bulk load).
     * It erases current index.
     */
    void fillWithBBoxes( GeoGrid* gg );

    /** Fills the index with the GeoGrid cell centers (bulk load).
     * It erases current index.
     * @param tolerance Sets the size of the bounding boxes around each cell center.
     */
    void fillWithCenters( GeoGrid* gg, double tolerance );

    /** Fills the index with the SegmentSet segments (bulk load).
     * It erases current index.
     * @param tolerance Sets the size of the bounding boxes around each segment.
     */
    void fill( SegmentSet* ss, double tolerance );

	/**
     * Returns the indexes of the n-nearest (in space) data lines to some data line given by its index.
	 * The indexes are the data record indexes (file data lines) of the DataFile used to fill
     * the index.
     */
    QList<uint> getNearest( uint index, uint n ) const;

	/**
     * Returns the indexes of the n-nearest (in space) data lines to a point in space.
	 * The indexes are the data record indexes (file data lines) of the DataFile used to fill
	 * the index.
	 */
    QList<uint> getNearest( double x, double y, double z, uint n ) const;

    /**
     * Returns the data line indexes of the n-nearest (in space) data lines within the given distance
     * to the point given by its index. The indexes are the data line indexes
     * (file data lines) of the DataFile used fill the index.  May return
     * an empty list.
     * @param distance The distance the returned points must be within.
     */
    QList<uint> getNearestWithin(uint index, uint n, double distance) const;

	/**
     * Returns the data line indexes of the n-nearest (in space) data lines within the given neighborhood
     * centered at given data cell (e.g. grid cell). The indexes are the data line indexes
     * (file data lines) of the DataFile used fill the index.  May return
     * an empty list.
	 */
    QList<uint> getNearestWithinGenericRTreeBased(const DataCell& dataCell,
                                        const SearchStrategy & searchStrategy ) const;

    /**
     * Returns the indexes of all the data lines within the given bounding box.
     * The indexes are the data line indexes (file data lines) of the DataFile
     * used fill the index.  May return an empty list.
     */
    QList<uint> getWithinBoundingBox( const BoundingBox& bbox ) const;

    /**
     * Does the same as getNearestWithinGenericRTreeBased() but is tuned for large, high-density data sets.
     * It may run slower for smaller data sets than the former, though.
     */
    QList<uint> getNearestWithinTunedForLargeDataSets(const DataCell& dataCell,
                                        const SearchStrategy & searchStrategy ) const;


    /**
     * It is a highly specialized member of the getNearest*() family of methods.
     * It works only with regular grid cells and Cartesian grids, taking
     * advantage of the implicit regular geometry and topology to improve search performance manifold.
     * An empty list can be returned.
     * @param gridCell The grid cell used as center to query neighboring cells.
     * @param searchStrategy The object containg the search parameters.
     * @param hasNDV Shortcut to avoid iterative calls to the slow DataFile::hasNoDataValue()
     * @param NDVvalue Shortcut to avoid iterative calls to the slow DataFile::getNoDataValueAsDouble()
     * @param The parallelepipedal search neighborhoord's size as number of cells in East-West direction.
     * @param The parallelepipedal search neighborhoord's size as number of cells in North-South direction.
     * @param The parallelepipedal search neighborhoord's size as number of cells in Up-Down direction.
     * @param simulatedData This should be set if this method is being called by computations that do not
     *                      immediately commit the results to the grid (e.g. simulation routines), otherwise an index
     *                      crash will ensue as the index in gridCell object is invalid or is -1.
     */
    QList<uint> getNearestFromCartesianGrid(const GridCell &gridCell,
                                            const SearchStrategy & searchStrategy,
                                            bool hasNDV,
                                            double NDVvalue,
                                            uint nCellsIDirection,
                                            uint nCellsJDirection,
                                            uint nCellsKDirection,
                                            const std::vector<double> *simulatedData = nullptr
                                            ) const;

    /**
     * Returns the data line indexes of the data lines that happen to be partially or entirely
     * within the given Z interval.  This query is useful, for example, to find data contained bewteen two
     * horizons or two well markers. The indexes are the data line indexes (file data lines) of the
     * DataFile used fill the index.
     */
    QList<uint> getWithinZInterval( double zInitial, double zFinal );

    /** Clears the spatial index. */
	void clear();

	/** Returns whether the spatial index has not been built. */
    bool isEmpty() const;

private:
	void setDataFile( DataFile* df );

	/** The R* variant of the rtree
	* WARNING: incorrect R-Tree parameter may lead to crashes with element insertions
	*/
    RStarRtree m_rtree; //TODO: make these parameters variable (passed in the constructor?)

	/** The data file which is being indexed. */
	DataFile* m_dataFile;
};

#endif // SPATIALINDEX_H
