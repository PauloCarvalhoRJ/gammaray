#ifndef TETRAHEDRON_H
#define TETRAHEDRON_H

#include <vector>
#include "geometry/vector3d.h"

/**
 * A Tetrahedron is a solid formed by four triangles.
 * Vertex order must form counter-clockwise faces as seen from inside the tetrahedron.
 *
 * vertex order and face relations
 *                                        face 0 = 0 1 2
 *                                        face 1 = 0 2 3
 *                           0            face 2 = 0 3 1
 *                         /  \\          face 3 = 1 3 2
 *                        |   |  \
 *                       /    |    \
 *                      |     |   _--1
 *                     /   ____\ /  /
 *                    | __/    |  /
 *                   //        |/
 *                  3--------- 2
 */
class Tetrahedron
{
public:
	Tetrahedron(); //makes v with four vertexes.

	std::vector<Vertex3D> v;

	double getVolume() const;
};

#endif // TETRAHEDRON_H
