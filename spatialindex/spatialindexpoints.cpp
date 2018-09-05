#include "spatialindexpoints.h"

#include <vector>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include "domain/pointset.h"
#include "domain/application.h"
#include "geostats/searchellipsoid.h"
#include "geostats/datacell.h"
#include "domain/cartesiangrid.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::point<double, 3, bg::cs::cartesian> Point3D;
typedef bg::model::box<Point3D> Box;
typedef std::pair<Box, size_t> Value;

// create the R* variant of the rtree
// WARNING: incorrect R-Tree parameter may lead to crashes with element insertions
bgi::rtree< Value, bgi::rstar<16,5,5,32> > rtree;

//the query data file
DataFile* dataFile = nullptr;

void setDataFile( DataFile* df ){
	dataFile = df;
    //loads the PointSet data.
	df->loadData();
}

SpatialIndexPoints::SpatialIndexPoints()
{

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
        rtree.insert( std::make_pair(box, iLine) );
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
		rtree.insert( std::make_pair(box, iLine) );
	}
}

QList<uint> SpatialIndexPoints::getNearest(uint index, uint n)
{
    QList<uint> result;

    //get the location of the point.
	double x = dataFile->getDataSpatialLocation( index, CartesianCoord::X );
	double y = dataFile->getDataSpatialLocation( index, CartesianCoord::Y );
	double z = dataFile->getDataSpatialLocation( index, CartesianCoord::Z );

    // find n nearest values to a point
    std::vector<Value> result_n;
    rtree.query(bgi::nearest(Point3D(x, y, z), n), std::back_inserter(result_n));

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
	double qx = dataFile->getDataSpatialLocation( index, CartesianCoord::X );
	double qy = dataFile->getDataSpatialLocation( index, CartesianCoord::Y );
	double qz = dataFile->getDataSpatialLocation( index, CartesianCoord::Z );
    Point3D qPoint(qx, qy, qz);

    //get the n-nearest points
    QList<uint> nearestSamples = SpatialIndexPoints::getNearest( index, n );

    //test the distance to each of the n-nearest points
    QList<uint>::iterator it = nearestSamples.begin();
    for(; it != nearestSamples.end(); ++it){
        //get the location of a near point.
        uint nIndex = *it;
		double nx = dataFile->getDataSpatialLocation( nIndex, CartesianCoord::X );
		double ny = dataFile->getDataSpatialLocation( nIndex, CartesianCoord::Y );
		double nz = dataFile->getDataSpatialLocation( nIndex, CartesianCoord::Z );
        //compute the distance between the query point and a nearest point
        double dist = boost::geometry::distance( qPoint, Point3D(nx, ny, nz) );
        if( dist < distance ){
            result.push_back( nIndex );
        }
    }
	return result;
}

QList<uint> SpatialIndexPoints::getNearestWithin(const DataCell& dataCell, uint n, const SearchNeighborhood & searchNeighborhood)
{
	QList<uint> result;

	//Get the location of the data cell.
	double x = dataCell._center._x;
	double y = dataCell._center._y;
	double z = 0.0; //put 2D data in the z==0.0 plane
	if( dataFile->isTridimensional() )
		z = dataCell._center._z;

	//Get the bounding box as a function of the search ellipsoid centered at the data cell.
	double maxX, maxY, maxZ, minX, minY, minZ;
    searchNeighborhood.getBBox( x, y, z, minX, minY, minZ, maxX, maxY, maxZ );
    Box searchBB( Point3D( minX, minY, minZ ),
				  Point3D( maxX, maxY, maxZ ));

	//Get all the points within the bounding box of the search ellipsoid.
	std::vector<Value> poinsInSearchBB;
    rtree.query( bgi::intersects( searchBB ), std::back_inserter(poinsInSearchBB) );

    //Traverse the result set.
	std::vector<Value>::iterator it = poinsInSearchBB.begin();
	bgi::rtree< Value, bgi::rstar<16,5,5,32> > rtreeLocal;
	for(; it != poinsInSearchBB.end(); ++it){
		uint indexP = (*it).second;
		//get the location of the point in the result set.
		double xP = dataFile->getDataSpatialLocation( indexP, CartesianCoord::X );
		double yP = dataFile->getDataSpatialLocation( indexP, CartesianCoord::Y );
		double zP = dataFile->getDataSpatialLocation( indexP, CartesianCoord::Z );
		//Test whether the point is actually inside the ellipsoid.
		if( searchNeighborhood.isInside( x, y, z, xP, yP, zP ) ){
			//Adds it to a local r-tree for the ensuing n-nearest search.
			rtreeLocal.insert( *it );
		}
	}

	//Get only the n-nearest of those found inside the ellipsoid.
	std::vector<Value> resultFinal;
	rtreeLocal.query(bgi::nearest(Point3D(x, y, z), n), std::back_inserter(resultFinal));

	//Collect the n-nearest point indexes found inside the ellipsoid.
	it = resultFinal.begin();
	for(; it != resultFinal.end(); ++it)
		result.push_back( (*it).second );

	return result;
}

void SpatialIndexPoints::clear()
{
    rtree.clear();
	dataFile = nullptr;
}
