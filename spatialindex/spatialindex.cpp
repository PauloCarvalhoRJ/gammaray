#include "spatialindex.h"

#include <vector>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include "domain/pointset.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::point<double, 3, bg::cs::cartesian> Point3D;
typedef bg::model::box<Point3D> Box;
typedef std::pair<Box, size_t> Value;

// create the R* variant of the rtree
bgi::rtree< Value, bgi::rstar<16> > rtree;

PointSet* pointset;

SpatialIndex::SpatialIndex()
{

}

void SpatialIndex::fill(PointSet *ps, double tolerance)
{
    pointset = ps;

    //first clear the index.
    clear();

    //loads the PointSet data.
    ps->loadData();

    //get the indexes for the X, Y and Z coordinates
    uint ix = ps->getXindex();
    uint iy = ps->getYindex();
    uint iz = ps->getZindex();

    //for each data line...
    uint totlines = ps->getDataLineCount();
    for( uint iLine = 0; iLine < totlines; ++iLine){
        //...make a Point3D for the index
        double x = ps->data( iLine, ix-1 ); //the returned indexes are GEO-EAS indexes (start with 1)
        double y = ps->data( iLine, iy-1 );
        double z = 0.0; //put 2D data in the z==0.0 plane
        if( iz > 0 )
            z = ps->data( iLine, iz-1 );
        //make a bounding box around the point.
        Box box( Point3D(x-tolerance, y-tolerance, z-tolerance),
                 Point3D(x+tolerance, y+tolerance, z+tolerance));
        //insert the box representing the point into the spatial index.
        rtree.insert( std::make_pair(box, iLine) );
    }
}

QList<uint> SpatialIndex::getNearest(uint index, uint n)
{
    QList<uint> result;

    //get the indexes for the X, Y and Z coordinates
    uint ix = pointset->getXindex();
    uint iy = pointset->getYindex();
    uint iz = pointset->getZindex();

    //get the location of the point.
    double x = pointset->data( index, ix );
    double y = pointset->data( index, iy );
    double z = 0.0; //put 2D data in the z==0.0 plane
    if( iz > 0 )
        z = pointset->data( index, iz-1 );

    // find n nearest values to a point
    std::vector<Value> result_n;
    rtree.query(bgi::nearest(Point3D(x, y, z), n), std::back_inserter(result_n));

    // collect the point indexes
    std::vector<Value>::iterator it = result_n.begin();
    for(; it != result_n.begin(); ++it){
        result.push_back( (*it).second );
    }

    //return the point indexes
    return result;
}

void SpatialIndex::clear()
{
    rtree.clear();
}
