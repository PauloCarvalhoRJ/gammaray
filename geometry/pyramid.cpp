#include "pyramid.h"

Pyramid::Pyramid() : v(5)
{
}

Tetrahedron Pyramid::getTetrahedron0() const
{
	Tetrahedron t;

	t.v[0].x = v[0].x;
	t.v[0].y = v[0].y;
	t.v[0].z = v[0].z;

	t.v[1].x = v[2].x;
	t.v[1].y = v[2].y;
	t.v[1].z = v[2].z;

	t.v[2].x = v[3].x;
	t.v[2].y = v[3].y;
	t.v[2].z = v[3].z;

	t.v[3].x = v[4].x;
	t.v[3].y = v[4].y;
	t.v[3].z = v[4].z;

	return t;
}

Tetrahedron Pyramid::getTetrahedron1() const
{
	Tetrahedron t;

	t.v[0].x = v[0].x;
	t.v[0].y = v[0].y;
	t.v[0].z = v[0].z;

	t.v[1].x = v[1].x;
	t.v[1].y = v[1].y;
	t.v[1].z = v[1].z;

	t.v[2].x = v[2].x;
	t.v[2].y = v[2].y;
	t.v[2].z = v[2].z;

	t.v[3].x = v[4].x;
	t.v[3].y = v[4].y;
	t.v[3].z = v[4].z;

	return t;
}

double Pyramid::getVolume() const
{
	return getTetrahedron0().getVolume() + getTetrahedron1().getVolume();
}
