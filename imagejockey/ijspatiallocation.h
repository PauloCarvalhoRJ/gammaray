#ifndef IJSPATIALLOCATION_H
#define IJSPATIALLOCATION_H

/** A location in space.  Essentially a structure to group x, y and z spatial coordinates. */
class IJSpatialLocation
{
public:

	/** Initializes the location at ( 0.0, 0.0, 0.0 ) */
	IJSpatialLocation();

	double _x;
	double _y;
	double _z;

	/** Returns this.x*other.y - this.y*other.x. Use only with 2D geometry computations. */
	double crossProduct2D( const IJSpatialLocation& otherLocation );

	/** Returns the distance between origin and this location in the XY plane. */
	double norm2D();
};

#endif // IJSPATIALLOCATION_H
