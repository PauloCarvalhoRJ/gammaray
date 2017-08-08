#ifndef SPATIALLOCATION_H
#define SPATIALLOCATION_H

/** A location in space.  Essentially a structure to group x, y and z spatial coordinates. */
class SpatialLocation
{
public:

    /** Initializes the location at ( 0.0, 0.0, 0.0 ) */
    SpatialLocation();

    double _x;
    double _y;
    double _z;

    /** Returns this.x*other.y - this.y*other.x. Use only with 2D geometry computations. */
    double crossProduct2D( const SpatialLocation& otherLocation );

    /** Returns the distance between origin and this location in the XY plane. */
    double norm2D();
};

#endif // SPATIALLOCATION_H
