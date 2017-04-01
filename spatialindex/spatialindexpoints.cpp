#include "spatialindexpoints.h"

#include <vector>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>

#include "domain/pointset.h"
#include "domain/application.h"

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::point<double, 3, bg::cs::cartesian> Point3D;
typedef bg::model::box<Point3D> Box;
typedef std::pair<Box, size_t> Value;

// create the R* variant of the rtree
// WARNING: incorrect R-Tree parameter may lead to crashes with element insertions
bgi::rtree< Value, bgi::rstar<16,5,5,32> > rtree;

//the query pointset
PointSet* pointset = nullptr;

//the GEO-EAS columns of the X, Y, Z variables of the point set file
int iX;
int iY;
int iZ;

void setPointSet( PointSet* ps ){
    pointset = ps;
    //loads the PointSet data.
    ps->loadData();
    //get the GEO-EAS indexes -1 for the X, Y and Z coordinates
    iX = ps->getXindex() - 1;
    iY = ps->getYindex() - 1;
    iZ = ps->getZindex() - 1;
}

SpatialIndexPoints::SpatialIndexPoints()
{

}

void SpatialIndexPoints::fill(PointSet *ps, double tolerance)
{
    //first clear the index.
    clear();

    setPointSet( ps );

    //for each data line...
    uint totlines = ps->getDataLineCount();
    for( uint iLine = 0; iLine < totlines; ++iLine){
        //...make a Point3D for the index
        double x = ps->data( iLine, iX );
        double y = ps->data( iLine, iY );
        double z = 0.0; //put 2D data in the z==0.0 plane
        if( iZ >= 0 )
            z = ps->data( iLine, iZ );
        //make a bounding box around the point.
        Box box( Point3D(x-tolerance, y-tolerance, z-tolerance),
                 Point3D(x+tolerance, y+tolerance, z+tolerance));
        //insert the box representing the point into the spatial index.
        rtree.insert( std::make_pair(box, iLine) );
    }
}

QList<uint> SpatialIndexPoints::getNearest(uint index, uint n)
{
    QList<uint> result;

    //get the location of the point.
    double x = pointset->data( index, iX );
    double y = pointset->data( index, iY );
    double z = 0.0; //put 2D data in the z==0.0 plane
    if( iZ >= 0 )
        z = pointset->data( index, iZ );

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
    double qx = pointset->data( index, iX );
    double qy = pointset->data( index, iY );
    double qz = 0.0; //put 2D data in the z==0.0 plane
    if( iZ >= 0 )
        qz = pointset->data( index, iZ );
    Point3D qPoint(qx, qy, qz);

    //get the n-nearest points
    QList<uint> nearestSamples = SpatialIndexPoints::getNearest( index, n );

    //test the distance to each of the n-nearest points
    QList<uint>::iterator it = nearestSamples.begin();
    for(; it != nearestSamples.end(); ++it){
        //get the location of a near point.
        uint nIndex = *it;
        double nx = pointset->data( nIndex, iX );
        double ny = pointset->data( nIndex, iY );
        double nz = 0.0; //put 2D data in the z==0.0 plane
        if( iZ >= 0 )
            nz = pointset->data( nIndex, iZ );
        //compute the distance between the query point and a nearest point
        double dist = boost::geometry::distance( qPoint, Point3D(nx, ny, nz) );
        if( dist < distance ){
            result.push_back( nIndex );
        }
    }
    return result;
}

void SpatialIndexPoints::clear()
{
    rtree.clear();
    pointset = nullptr;
}
