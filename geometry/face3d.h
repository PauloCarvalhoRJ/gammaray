#ifndef FACE3D_H
#define FACE3D_H

#include "geometry/vector3d.h"
#include <vector>

class Face3D {
public:
	Face3D(); //making the face with 4 vertexes by default

	std::vector<Vertex3D> v;

	Vector3D normal() const;

	/** Computes the distance from point to the plane that contains this face. */
	double distance( const Vertex3D& point ) const;
};

#endif // FACE3D_H
