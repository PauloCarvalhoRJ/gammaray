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
};

#endif // SPATIALLOCATION_H
