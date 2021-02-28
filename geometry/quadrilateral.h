#ifndef QUADRILATERAL_H
#define QUADRILATERAL_H

#include <vector>
#include "geometry/vector3d.h"
#include "geometry/triangle.h"

/**
 *
 * Vertex indexes in v and edge relations:
 *
 *      3-----------2   edge 0 = 0 1
 *      |           |   edge 1 = 1 2
 *      |           |   edge 2 = 2 3
 *      |           |   edge 3 = 3 0
 *      |           |
 *      |           |
 *      0-----------1
 *
 */
class Quadrilateral
{
public:
    Quadrilateral(); //makes v with room for four vertexes.

    std::vector<Vertex3D> v;

    ///////The two triangles below form this quadrilateral when joinded. /////
    /** Makes a Triangle from vertexes 0, 1, 3 (in this order). */
    Triangle getTriangle0() const;
    /** Makes a Triangle from vertexes 1, 2, 3 (in this order). */
    Triangle getTriangle1() const;

    /** Returns the area of this quadrilateral. */
    double getArea() const;
};

#endif // QUADRILATERAL_H
