#include "tetrahedron.h"
#include "geostats/matrix3x3.h"
#include "util.h"
#include <cmath>

Tetrahedron::Tetrahedron() : v(4)
{
}


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


double Tetrahedron::getVolume() const
{
	Matrix3X3<double> m3;
	Vertex3D row1 = v[0] - v[3];
	Vertex3D row2 = v[1] - v[3];
	Vertex3D row3 = v[2] - v[3];
	m3._a11 = row1.x; m3._a12 = row1.y; m3._a13 = row1.z;
	m3._a21 = row2.x; m3._a22 = row2.y; m3._a23 = row2.z;
	m3._a31 = row3.x; m3._a32 = row3.y; m3._a33 = row3.z;
    return std::abs( m3.det() / 6.0 );
}

bool Tetrahedron::isInside(const Vertex3D queryPoint) const
{
    return isSameSide(v[0], v[1], v[2], v[3], queryPoint) &&
           isSameSide(v[1], v[2], v[3], v[0], queryPoint) &&
           isSameSide(v[2], v[3], v[0], v[1], queryPoint) &&
           isSameSide(v[3], v[0], v[1], v[2], queryPoint);
}

bool Tetrahedron::isSameSide(const Vertex3D v1,
                             const Vertex3D v2,
                             const Vertex3D v3,
                             const Vertex3D referenceVertex,
                             const Vertex3D queryPoint) const
{
    Vector3D normalOfPlane    = ( v2 - v1 ).cross( v3 - v1 );
    double dotReferenceVertex = normalOfPlane.dot( referenceVertex - v1 );
    double dotQueryPoint      = normalOfPlane.dot( queryPoint      - v1 );
    return Util::sign(dotReferenceVertex) == Util::sign(dotQueryPoint);
}
