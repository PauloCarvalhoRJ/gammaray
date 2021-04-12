#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <vector>
#include "geometry/vector3d.h"

/**
 *
 * vertex order in v and edge relations
 *
 *                                        edge 0 = 0 1
 *                                        edge 1 = 1 2
 *                           0            edge 2 = 2 0
 *                         /  \
 *                        |   |
 *                       /    |
 *                      |     |
 *                     /       \
 *                    |        |
 *                   /         |
 *                  1--------- 2
 */
class Triangle
{
public:
    Triangle(); //makes v with three vertexes.

    std::vector<Vertex3D> v;

    /** Returns the area of this triangle using Heron's formula.
     *  ref: http://mathcentral.uregina.ca/QQ/database/QQ.09.01/bat1.html (accessed 2021-02-28)
     */
    double getArea() const;
};

#endif // TRIANGLE_H
