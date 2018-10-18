#include "face3d.h"
#include <cassert>

Face3D::Face3D() : v(4){}

Vector3D Face3D::normal() const {
	assert(v.size() > 2);
	Vector3D dir1 = v[1] - v[0];
	Vector3D dir2 = v[2] - v[0];
	Vector3D n  = dir1.cross(dir2);
	double d = n.norm();
	return Vector3D{n.x / d, n.y / d, n.z / d};
}

double Face3D::distance(const Vertex3D & point) const
{
	//Get the vector normal (n) to the plane of this face.
	Vector3D n = normal();
	//Plane of triangle:
	//     n . u = n . v[0], where u is any (x,y,z) contained in the plane (. == dot product).
	//     v[0] is a vertex of the face.
	//Line that contains the point with direction given by the normal (n):
	//     u = point + tn (t is a scalar)
	//Intersection point satisfies:
	//     n . ( point + tn ) = n . v[0]
	//     n . point + t      = n . v[0]
	double t                  = n.dot( v[0] ) - n.dot( point );
	//Hence, the point that intersects the line and the plane, p0, is:
	Vertex3D p0 = point + t * n;
	//and the distance is point-to-p0 distance
	return ( point - p0 ).norm();
}
