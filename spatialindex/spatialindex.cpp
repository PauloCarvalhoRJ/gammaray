#include "spatialindex.h"

#include "domain/pointset.h"
#include "domain/application.h"
#include "geostats/searchellipsoid.h"
#include "geostats/datacell.h"
#include "geostats/searchstrategy.h"
#include "domain/cartesiangrid.h"
#include "domain/geogrid.h"
#include "domain/segmentset.h"

#include <cassert>


void SpatialIndex::setDataFile( DataFile* df ){
	m_dataFile = df;
    //loads the PointSet data.
	df->loadData();
}

SpatialIndex::SpatialIndex() :
	m_dataFile( nullptr )
{
}

SpatialIndex::~SpatialIndex()
{
    //clears the global variable with the index after usage
    clear();
}

void SpatialIndex::fill(PointSet *ps, double tolerance)
{
    //first clear the index.
    clear();

	setDataFile( ps );

    //for each data line...
    uint totlines = ps->getDataLineCount();
    if( totlines == 0 )
        Application::instance()->logWarn("SpatialIndex::fill(PointSet *, double): no data.  Make sure data was loaded prior to indexing.");

    std::vector< Value > boxes;
    boxes.reserve( totlines );

    for( uint iLine = 0; iLine < totlines; ++iLine){
        //...make a Point3D for the index
		double x = ps->getDataSpatialLocation( iLine, CartesianCoord::X );
		double y = ps->getDataSpatialLocation( iLine, CartesianCoord::Y );
		double z = ps->getDataSpatialLocation( iLine, CartesianCoord::Z );
        //make a bounding box around the point.
        Box box( Point3D(x-tolerance, y-tolerance, z-tolerance),
                 Point3D(x+tolerance, y+tolerance, z+tolerance));
        //insert the box representing the point into the spatial index.
        //m_rtree.insert( std::make_pair(box, iLine) );
        boxes.push_back( std::make_pair(box, iLine) );
    }

    //building the tree like this makes use of the packing algorithm (faster bulk load)
    m_rtree = RStarRtree( boxes );
}

void SpatialIndex::fill(CartesianGrid * cg)
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
    if( totlines == 0 )
        Application::instance()->logWarn("SpatialIndex::fill(CartesianGrid *): no data.  Make sure data was loaded prior to indexing.");

    std::vector< Value > boxes;
    boxes.reserve( totlines );

    for( uint iLine = 0; iLine < totlines; ++iLine){
		//...make a Point3D for the index
		double x = cg->getDataSpatialLocation( iLine, CartesianCoord::X );
		double y = cg->getDataSpatialLocation( iLine, CartesianCoord::Y );
		double z = cg->getDataSpatialLocation( iLine, CartesianCoord::Z );
		//make a bounding box around the point.
		Box box( Point3D(x-tX, y-tY, z-tZ),
				 Point3D(x+tX, y+tY, z+tZ) );
		//insert the box representing the point into the spatial index.
        //m_rtree.insert( std::make_pair(box, iLine) );
        boxes.push_back( std::make_pair(box, iLine) );
	}

    m_rtree = RStarRtree( boxes );
}

void SpatialIndex::fill(GeoGrid * gg)
{
	//first clear the index.
	clear();

	//set the data file as the passed GeoGrid
	setDataFile( gg );

	//load the GeoGrid's mesh
	gg->loadMesh();

	//for each data line...
	uint totlines = gg->getDataLineCount();
    if( totlines == 0 )
        Application::instance()->logWarn("SpatialIndex::fill(GeoGrid *): no data.  Make sure data was loaded prior to indexing.");

    std::vector< Value > boxes;
    boxes.reserve( totlines );

    for( uint iLine = 0; iLine < totlines; ++iLine){
		//get the cell's bounding box (each line corresponds to a cell)
		double minX, minY, minZ, maxX, maxY, maxZ;
		gg->getBoundingBox( iLine, minX, minY, minZ, maxX, maxY, maxZ );
		//make the bounding box object
		Box box( Point3D(minX, minY, minZ),
				 Point3D(maxX, maxY, maxZ) );
		//insert the box representing the point into the spatial index.
        //m_rtree.insert( std::make_pair(box, iLine) );
        boxes.push_back( std::make_pair(box, iLine) );
    }

    m_rtree = RStarRtree( boxes );
}

void SpatialIndex::fill( SegmentSet *ss, double tolerance )
{
    //first clear the index.
    clear();

    //set the data file as the passed SegmentSet
    setDataFile( ss );

    //for each data line...
    uint totlines = ss->getDataLineCount();
    if( totlines == 0 )
        Application::instance()->logWarn("SpatialIndex::fill(SegmentSet *, double): no data.  Make sure data was loaded prior to indexing.");

    std::vector< Value > boxes;
    boxes.reserve( totlines );

    for( uint iLine = 0; iLine < totlines; ++iLine){
        //get the segment's bounding box (each line corresponds to a segment)
        double minX, minY, minZ, maxX, maxY, maxZ;
        ss->getBoundingBox( iLine, minX, minY, minZ, maxX, maxY, maxZ );
        //make the bounding box object
        Box box( Point3D(minX-tolerance, minY-tolerance, minZ-tolerance),
                 Point3D(maxX+tolerance, maxY+tolerance, maxZ+tolerance) );
        //insert the box representing the segment into the spatial index.
        //m_rtree.insert( std::make_pair(box, iLine) );
        boxes.push_back( std::make_pair(box, iLine) );
    }

    m_rtree = RStarRtree( boxes );
}

QList<uint> SpatialIndex::getNearest(uint index, uint n)
{
    assert( m_dataFile && "SpatialIndex::getNearest(): No data file.  Make sure you have made a call to fill() prior to making queries.");

    QList<uint> result;
    result.reserve( n );

    //get the location of the point.
	double x = m_dataFile->getDataSpatialLocation( index, CartesianCoord::X );
	double y = m_dataFile->getDataSpatialLocation( index, CartesianCoord::Y );
	double z = m_dataFile->getDataSpatialLocation( index, CartesianCoord::Z );

    // find n nearest values to a point
    std::vector<Value> result_n;
    result_n.reserve( n );
	m_rtree.query(bgi::nearest(Point3D(x, y, z), n), std::back_inserter(result_n));

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

QList<uint> SpatialIndex::getNearest(double x, double y, double z, uint n)
{
    assert( m_dataFile && "SpatialIndex::getNearest(): No data file.  Make sure you have made a call to fill() prior to making queries.");

	QList<uint> result;
    result.reserve( n );

	// find n nearest values to a point
	std::vector<Value> result_n;
    result_n.reserve( n );
	m_rtree.query(bgi::nearest(Point3D(x, y, z), n), std::back_inserter(result_n));

	// collect the point indexes
	std::vector<Value>::iterator it = result_n.begin();
	for(; it != result_n.end(); ++it){
		result.push_back( (*it).second );
	}

	//return the point indexes
	return result;
}

QList<uint> SpatialIndex::getNearestWithin(uint index, uint n, double distance )
{
    assert( m_dataFile && "SpatialIndex::getNearestWithin(): No data file.  Make sure you have made a call to fill() prior to making queries.");

	QList<uint> result;
    result.reserve( n );

    //get the location of the query point.
	double qx = m_dataFile->getDataSpatialLocation( index, CartesianCoord::X );
	double qy = m_dataFile->getDataSpatialLocation( index, CartesianCoord::Y );
	double qz = m_dataFile->getDataSpatialLocation( index, CartesianCoord::Z );
    Point3D qPoint(qx, qy, qz);

    //get the n-nearest points
    QList<uint> nearestSamples = SpatialIndex::getNearest( index, n );

    //test the distance to each of the n-nearest points
    QList<uint>::iterator it = nearestSamples.begin();
    for(; it != nearestSamples.end(); ++it){
        //get the location of a near point.
        uint nIndex = *it;
		double nx = m_dataFile->getDataSpatialLocation( nIndex, CartesianCoord::X );
		double ny = m_dataFile->getDataSpatialLocation( nIndex, CartesianCoord::Y );
		double nz = m_dataFile->getDataSpatialLocation( nIndex, CartesianCoord::Z );
        //compute the distance between the query point and a nearest point
        double dist = boost::geometry::distance( qPoint, Point3D(nx, ny, nz) );
        if( dist < distance ){
            result.push_back( nIndex );
        }
    }
	return result;
}

QList<uint> SpatialIndex::getNearestWithin(const DataCell& dataCell, const SearchStrategy & searchStrategy)
{
    assert( m_dataFile && "SpatialIndex::getNearestWithin(): No data file.  Make sure you have made a call to fill() prior to making queries.");
	//TODO: Possible Refactoring: some of the logic in here may in fact belong to the SearchStrategy class.

	//get the desired number of samples.
	uint n = searchStrategy.m_nb_samples;

    QList<uint> result;
    result.reserve( n );

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
	if( m_dataFile->isTridimensional() )
		z = dataCell._center._z;

    //Get the bounding box as a function of the search neighborhood centered at the data cell.
	double maxX, maxY, maxZ, minX, minY, minZ;
    searchNeighborhood.getBBox( x, y, z, minX, minY, minZ, maxX, maxY, maxZ );
    Box searchBB( Point3D( minX, minY, minZ ),
				  Point3D( maxX, maxY, maxZ ));

    //Get all the points within the bounding box of the search neighborhood.
    //This step improves performance because the actual inside/outside test of the search
    //neighborhood implementation may be slow.
    std::vector<Value> pointsInSearchBB;
    pointsInSearchBB.reserve( 1000000 ); //make enormous room just in case the user set a big search neighborhood for a dense data set
    m_rtree.query( bgi::intersects( searchBB ), std::back_inserter(pointsInSearchBB) );

    return result;

    //Get all the samples actually inside the search neighborhood.
    std::vector<Value>::iterator it = pointsInSearchBB.begin();
    typedef bgi::rtree< Value, bgi::rstar<16,5,5,32> > RTreeLocal;
    RTreeLocal rtreeLocal;
	std::vector<Value> resultMinDist;
	resultMinDist.reserve( 1 );
    for(; it != pointsInSearchBB.end(); ++it){
		uint indexP = (*it).second;
		//get the location of the point in the result set.
		double xP = m_dataFile->getDataSpatialLocation( indexP, CartesianCoord::X );
		double yP = m_dataFile->getDataSpatialLocation( indexP, CartesianCoord::Y );
		double zP = m_dataFile->getDataSpatialLocation( indexP, CartesianCoord::Z );
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
					double xClosestP = m_dataFile->getDataSpatialLocation( indexClosestP, CartesianCoord::X );
					double yClosestP = m_dataFile->getDataSpatialLocation( indexClosestP, CartesianCoord::Y );
					double zClosestP = m_dataFile->getDataSpatialLocation( indexClosestP, CartesianCoord::Z );
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
			double x = m_dataFile->getDataSpatialLocation( (*it).second, CartesianCoord::X );
			double y = m_dataFile->getDataSpatialLocation( (*it).second, CartesianCoord::Y );
			double z = m_dataFile->getDataSpatialLocation( (*it).second, CartesianCoord::Z );
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
        resultNNearest.reserve( 1000000 ); //make enormous room just in case the user set a big search neighborhood for a dense data set
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

void SpatialIndex::clear()
{
	m_rtree.clear();
	m_dataFile = nullptr;
}

bool SpatialIndex::isEmpty()
{
	return m_rtree.empty();
}
