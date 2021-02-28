#include "triangle.h"
#include <cmath>

Triangle::Triangle() : v(3)
{

}

double Triangle::getArea() const
{
    //Compute the lengths of the edges.
    double lengthEdge0 = ( v[1] - v[0] ).norm();
    double lengthEdge1 = ( v[2] - v[1] ).norm();
    double lengthEdge2 = ( v[0] - v[2] ).norm();

    //Compute the semi-perimeter.
    double s = ( lengthEdge0 + lengthEdge1 + lengthEdge2 ) / 2.0;

    //Apply Heron's formula.
    return std::sqrt( s * ( s - lengthEdge0 ) * ( s - lengthEdge1 ) * ( s - lengthEdge2 ) );
}
