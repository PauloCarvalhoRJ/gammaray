#include "spatialindexpoints.h"

#include <vector>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include "domain/pointset.h"
#include "domain/application.h"
#include "geostats/searchellipsoid.h"
#include "geostats/datacell.h"
#include "geostats/searchstrategy.h"
#include "domain/cartesiangrid.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::point<double, 3, bg::cs::cartesian> Point3D;
typedef bg::model::box<Point3D> Box;
typedef std::pair<Box, size_t> Value;

// create the R* variant of the rtree
// WARNING: incorrect R-Tree parameter may lead to crashes with element insertions
bgi::rtree< Value, bgi::rstar<16,5,5,32> > g_rtree;

//the query data file
DataFile* g_dataFile = nullptr;

void setDataFile( DataFile* df ){
	g_dataFile = df;
    //loads the PointSet data.
	df->loadData();
}

//A guard against simulatneous use of the spatial index.
bool g_spatialIndexBeingUsed = false;

SpatialIndexPoints::SpatialIndexPoints()
{
	assert( ! g_spatialIndexBeingUsed && "Spatial index being used elsewhere.  Make sure instances of SpatialIndexPoints are being destroyed immediately after use." );
	g_spatialIndexBeingUsed = true;
}

SpatialIndexPoints::~SpatialIndexPoints()
{
	g_spatialIndexBeingUsed = false;
    //clears the global variable with the index after usage
    clear();
}

void SpatialIndexPoints::fill(PointSet *ps, double tolerance)
{
    //first clear the index.
    clear();

	setDataFile( ps );

    //for each data line...
    uint totlines = ps->getDataLineCount();
    for( uint iLine = 0; iLine < totlines; ++iLine){
        //...make a Point3D for the index
		double x = ps->getDataSpatialLocation( iLine, CartesianCoord::X );
		double y = ps->getDataSpatialLocation( iLine, CartesianCoord::Y );
		double z = ps->getDataSpatialLocation( iLine, CartesianCoord::Z );
        //make a bounding box around the point.
        Box box( Point3D(x-tolerance, y-tolerance, z-tolerance),
                 Point3D(x+tolerance, y+tolerance, z+tolerance));
        //insert the box representing the point into the spatial index.
		g_rtree.insert( std::make_pair(box, iLine) );
	}
}

void SpatialIndexPoints::fill(CartesianGrid * cg)
{
	//first clear the index.
	clear();

	setDataFile( cg );

	//the search tolerance is half of cell sizes.
	double tX = cg->getDX() / 2;
	double tY = cg->getDY() / 2;
	double tZ = cg->getDZ() / 2;

	//for each data line...
	uint totlines = cg->getDataLineCount();
	for( uint iLine = 0; iLine < totlines; ++iLine){
		//...make a Point3D for the index
		double x = cg->getDataSpatialLocation( iLine, CartesianCoord::X );
		double y = cg->getDataSpatialLocation( iLine, CartesianCoord::Y );
		double z = cg->getDataSpatialLocation( iLine, CartesianCoord::Z );
		//make a bounding box around the point.
		Box box( Point3D(x-tX, y-tY, z-tZ),
				 Point3D(x+tX, y+tY, z+tZ) );
		//insert the box representing the point into the spatial index.
		g_rtree.insert( std::make_pair(box, iLine) );
	}
}

QList<uint> SpatialIndexPoints::getNearest(uint index, uint n)
{
    QList<uint> result;

    //get the location of the point.
	double x = g_dataFile->getDataSpatialLocation( index, CartesianCoord::X );
	double y = g_dataFile->getDataSpatialLocation( index, CartesianCoord::Y );
	double z = g_dataFile->getDataSpatialLocation( index, CartesianCoord::Z );

    // find n nearest values to a point
    std::vector<Value> result_n;
	g_rtree.query(bgi::nearest(Point3D(x, y, z), n), std::back_inserter(result_n));

    // collect the point indexes
    std::vector<Value>::iterator it = result_n.begin();
    for(; it != result_n.end(); ++it){
        //do not return itself
        if( index != (*it).second )
            result.push_back( (*it).second );
    }

    //return the point indexes
    return result;
}

QList<uint> SpatialIndexPoints::getNearestWithin(uint index, uint n, double distance )
{
    QList<uint> result;

    //get the location of the query point.
	double qx = g_dataFile->getDataSpatialLocation( index, CartesianCoord::X );
	double qy = g_dataFile->getDataSpatialLocation( index, CartesianCoord::Y );
	double qz = g_dataFile->getDataSpatialLocation( index, CartesianCoord::Z );
    Point3D qPoint(qx, qy, qz);

    //get the n-nearest points
    QList<uint> nearestSamples = SpatialIndexPoints::getNearest( index, n );

    //test the distance to each of the n-nearest points
    QList<uint>::iterator it = nearestSamples.begin();
    for(; it != nearestSamples.end(); ++it){
        //get the location of a near point.
        uint nIndex = *it;
		double nx = g_dataFile->getDataSpatialLocation( nIndex, CartesianCoord::X );
		double ny = g_dataFile->getDataSpatialLocation( nIndex, CartesianCoord::Y );
		double nz = g_dataFile->getDataSpatialLocation( nIndex, CartesianCoord::Z );
        //compute the distance between the query point and a nearest point
        double dist = boost::geometry::distance( qPoint, Point3D(nx, ny, nz) );
        if( dist < distance ){
            result.push_back( nIndex );
        }
    }
	return result;
}

QList<uint> SpatialIndexPoints::getNearestWithin(const DataCell& dataCell, const SearchStrategy & searchStrategy)
{
	//TODO: Possible Refactoring: some of the logic in here may in fact belong to the SearchStrategy class.

	QList<uint> result;

	//get the desired number of samples.
	uint n = searchStrategy.m_nb_samples;

	//get the search neighboorhood (e.g. an ellipsoid).
	const SearchNeighborhood& searchNeighborhood = *(searchStrategy.m_searchNB);

	//get the minimum distance between samples. (0.0 == not used)
	double minDist = searchStrategy.m_minDistanceBetweenSamples;

	//set a flag to avoid computing distances unnecessarily (performance reason).
	bool useMinDist = minDist > 0.0;

	//Get the location of the data cell.
	double x = dataCell._center._x;
	double y = dataCell._center._y;
	double z = 0.0; //put 2D data in the z==0.0 plane
	if( g_dataFile->isTridimensional() )
		z = dataCell._center._z;

    //Get the bounding box as a function of the search neighborhood centered at the data cell.
	double maxX, maxY, maxZ, minX, minY, minZ;
    searchNeighborhood.getBBox( x, y, z, minX, minY, minZ, maxX, maxY, maxZ );
    Box searchBB( Point3D( minX, minY, minZ ),
				  Point3D( maxX, maxY, maxZ ));

    //Get all the points within the bounding box of the search neighborhood.
    //This step improves performance because the actual inside/outside test of the search
    //neighborhood implementation may be slow.
	std::vector<Value> poinsInSearchBB;
	g_rtree.query( bgi::intersects( searchBB ), std::back_inserter(poinsInSearchBB) );

    //Get all the samples actually inside the search neighborhood.
	std::vector<Value>::iterator it = poinsInSearchBB.begin();
    typedef bgi::rtree< Value, bgi::rstar<16,5,5,32> > RTreeLocal;
    RTreeLocal rtreeLocal;
	std::vector<Value> resultMinDist;
	resultMinDist.reserve( 1 );
	for(; it != poinsInSearchBB.end(); ++it){
		uint indexP = (*it).second;
		//get the location of the point in the result set.
		double xP = g_dataFile->getDataSpatialLocation( indexP, CartesianCoord::X );
		double yP = g_dataFile->getDataSpatialLocation( indexP, CartesianCoord::Y );
		double zP = g_dataFile->getDataSpatialLocation( indexP, CartesianCoord::Z );
		//Test whether the point is actually inside the ellipsoid.
		if( searchNeighborhood.isInside( x, y, z, xP, yP, zP ) ){
			//if it necessary to impose a minimum distance between samples...
			if( useMinDist ){
				//...and if there is at least a sample already collected...
				if( ! rtreeLocal.empty() ){
					//...then query the local r-tree for the closest sample to the current sample so far.
					rtreeLocal.query(bgi::nearest(Point3D(xP, yP, zP), 1), std::back_inserter(resultMinDist));
					//get the location of the closest sample already collected.
					uint indexClosestP = resultMinDist[0].second;
					double xClosestP = g_dataFile->getDataSpatialLocation( indexClosestP, CartesianCoord::X );
					double yClosestP = g_dataFile->getDataSpatialLocation( indexClosestP, CartesianCoord::Y );
					double zClosestP = g_dataFile->getDataSpatialLocation( indexClosestP, CartesianCoord::Z );
					//reset the vector used to collect the nearest sample already collected
					resultMinDist.clear();
					//compute the distance between the current sample and the nearest sample collected
					double dist = boost::geometry::distance( Point3D(xP, yP, zP), Point3D(xClosestP, yClosestP, zClosestP) );
					//if the current sample is closer to a sample previously collected than allowed...
					if( dist < minDist ){
						//...skip to the next sample (current sample is not collected).
						continue;
					}
				}
			}
			//Adds the current sample to a local r-tree for the ensuing n-nearest search.
			rtreeLocal.insert( *it );
		}
	}

	//The search strategy may need to perform spatial filtering (e.g. octant/sector search) of the samples found
	//in the search neighborhood.
	if( searchStrategy.NBhasSpatialFiltering() ){
        //Copy all sample locations found inside the neighborhood to a vector.
		std::vector<IndexedSpatialLocationPtr> locationsToFilter;
        locationsToFilter.reserve( rtreeLocal.size() );
        for ( RTreeLocal::const_iterator it = rtreeLocal.begin() ; it != rtreeLocal.end() ; ++it ){
            //Get sample's location given the index stored in the r-tree.
            double x = g_dataFile->getDataSpatialLocation( (*it).second, CartesianCoord::X );
            double y = g_dataFile->getDataSpatialLocation( (*it).second, CartesianCoord::Y );
            double z = g_dataFile->getDataSpatialLocation( (*it).second, CartesianCoord::Z );
			locationsToFilter.push_back( IndexedSpatialLocationPtr( new IndexedSpatialLocation( x, y, z, (*it).second ) ) );
        }
		//Perform spatial filter with respect to the center of the current estimation cell.
		searchStrategy.m_searchNB->performSpatialFilter( x, y, z, locationsToFilter, searchStrategy );
		//...Collect the indexes of the samples spatially filtered.
		std::vector<IndexedSpatialLocationPtr>::iterator it = locationsToFilter.begin();
		for(; it != locationsToFilter.end(); ++it)
			result.push_back( (*it)->_index );
	//Otherwise, simply get the n-nearest of those found inside the neighborhood.
	} else {
		std::vector<Value> resultNNearest;
		rtreeLocal.query(bgi::nearest(Point3D(x, y, z), n), std::back_inserter(resultNNearest));
		//If the number of n-neares samples found is greater than or equal the minimum number of samples
		//set in search strategy...
		if( resultNNearest.size() >= searchStrategy.m_minNumberOfSamples ) {
			//...Collect the n-nearest point indexes found inside the ellipsoid.
			it = resultNNearest.begin();
			for(; it != resultNNearest.end(); ++it)
				result.push_back( (*it).second );
		}
	}


	return result;
}

void SpatialIndexPoints::clear()
{
	g_rtree.clear();
	g_dataFile = nullptr;
}
