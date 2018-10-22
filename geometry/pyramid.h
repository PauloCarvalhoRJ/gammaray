#ifndef PYRAMID_H
#define PYRAMID_H

#include <vector>
#include "geometry/vector3d.h"
#include "geometry/tetrahedron.h"

/**
 * A Pyramid is a solid formed by four triangles and a quadrilateral base.
 * Vertex order must form counter-clockwise faces as seen from inside the pyramid.
 *
 * vertex order and face relations
 *                                        vertex 0 = apex
 *                                        face 0 = 0 1 2
 *                                        face 1 = 0 2 3
 *                           0            face 2 = 0 3 4
 *                         // \\          face 3 = 1 4 3 2 (base)
 *                        ||  |  \
 *                       / /  |    \
 *                      | 1---|------2
 *                     / /     \    /
 *                    |/       |  /
 *                   /         |/
 *                  4--------- 3
 */
class Pyramid
{
public:
	Pyramid(); //makes v with five vertexes.

	std::vector<Vertex3D> v;

	///////The two tetrahedra below form this pyramid when joinded. /////
	/** Makes a tetrahedron from vertexes 0, 2, 3 and 4 (in this order). */
	Tetrahedron getTetrahedron0() const;
	/** Makes a tetrahedron from vertexes 0, 1, 2 and 4 (in this order). */
	Tetrahedron getTetrahedron1() const;

	/** Returns the volume of this pyramid. */
	double getVolume() const;
};

#endif // PYRAMID_H
