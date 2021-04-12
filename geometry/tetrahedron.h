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

    bool isInside( const Vertex3D queryPoint ) const;

protected:
    /** Returns whether queryPoint is on the same side of referenceVertex with respect to
     * the plane defined by v1, v2 and v3.
     */
    bool isSameSide( const Vertex3D v1,
                     const Vertex3D v2,
                     const Vertex3D v3,
                     const Vertex3D referenceVertex,
                     const Vertex3D queryPoint ) const;
};

#endif // TETRAHEDRON_H
