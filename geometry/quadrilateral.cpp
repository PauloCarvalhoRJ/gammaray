#include "quadrilateral.h"

Quadrilateral::Quadrilateral() : v(4)
{

}

Triangle Quadrilateral::getTriangle0() const
{
    Triangle t;

    t.v[0].x = v[0].x;
    t.v[0].y = v[0].y;
    t.v[0].z = v[0].z;

    t.v[1].x = v[1].x;
    t.v[1].y = v[1].y;
    t.v[1].z = v[1].z;

    t.v[2].x = v[3].x;
    t.v[2].y = v[3].y;
    t.v[2].z = v[3].z;

    return t;
}

Triangle Quadrilateral::getTriangle1() const
{
    Triangle t;

    t.v[0].x = v[1].x;
    t.v[0].y = v[1].y;
    t.v[0].z = v[1].z;

    t.v[1].x = v[2].x;
    t.v[1].y = v[2].y;
    t.v[1].z = v[2].z;

    t.v[2].x = v[3].x;
    t.v[2].y = v[3].y;
    t.v[2].z = v[3].z;

    return t;
}

double Quadrilateral::getArea() const
{
    return getTriangle0().getArea() + getTriangle1().getArea();
}
