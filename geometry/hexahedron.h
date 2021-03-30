#ifndef HEXAHEDRON_H
#define HEXAHEDRON_H

#include <vector>
#include "geometry/vector3d.h"
#include "geometry/pyramid.h"

/**
 *
 * Vertex indexes in v and face relations:
 *
 *
 *
 *                        3-----------2   face 0 = 0 1 2 3
 *                       /|          /|   face 1 = 4 7 6 5
 *                     /  |        /  |   face 2 = 0 3 7 4
 *                   /    |      /    |   face 3 = 1 5 6 2
 *                  7--------- 6      |   face 4 = 0 4 5 1
 *                  |     |    |      |   face 5 = 3 2 6 7
 *                  |     0----|------1
 *                  |    /     |     /
 *                  |  /       |   /
 *                  |/         | /
 *                  4--------- 5
 *
 *
 */
class Hexahedron
{
public:
	Hexahedron(); //makes v with room for eight vertexes.

	std::vector<Vertex3D> v;

	///////The three pyramids below form this hexahedron when joinded. /////
	/** Makes a Pyramid from vertexes 3, 1, 2, 6 and 5 (in this order). */
	Pyramid getPyramid0() const;
	/** Makes a Pyramid from vertexes 3, 4, 5, 6 and 7 (in this order). */
	Pyramid getPyramid1() const;
	/** Makes a Pyramid from vertexes 3, 0, 1, 5 and 4 (in this order). */
	Pyramid getPyramid2() const;

	/** Returns the volume of this hexahedron. */
	double getVolume() const;

    /** Returns whether the given pouint is inside the hexahedron.
     *  This test does not require a specific winding order like
     *  the potentially faster Util::isInside().
     */
    bool isInside( const Vertex3D point ) const;
};

#endif // HEXAHEDRON_H
