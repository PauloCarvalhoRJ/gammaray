#include "hexahedron.h"

Hexahedron::Hexahedron() : v(8)
{
}



Pyramid Hexahedron::getPyramid0() const
{
	Pyramid p;

	p.v[0].x = v[3].x;
	p.v[0].y = v[3].y;
	p.v[0].z = v[3].z;

	p.v[2].x = v[2].x;
	p.v[2].y = v[2].y;
	p.v[2].z = v[2].z;

	p.v[1].x = v[1].x;
	p.v[1].y = v[1].y;
	p.v[1].z = v[1].z;

	p.v[4].x = v[5].x;
	p.v[4].y = v[5].y;
	p.v[4].z = v[5].z;

	p.v[3].x = v[6].x;
	p.v[3].y = v[6].y;
	p.v[3].z = v[6].z;

	return p;
}

Pyramid Hexahedron::getPyramid1() const
{
	Pyramid p;

	p.v[0].x = v[3].x;
	p.v[0].y = v[3].y;
	p.v[0].z = v[3].z;

	p.v[4].x = v[7].x;
	p.v[4].y = v[7].y;
	p.v[4].z = v[7].z;

	p.v[3].x = v[6].x;
	p.v[3].y = v[6].y;
	p.v[3].z = v[6].z;

	p.v[1].x = v[4].x;
	p.v[1].y = v[4].y;
	p.v[1].z = v[4].z;

	p.v[2].x = v[5].x;
	p.v[2].y = v[5].y;
	p.v[2].z = v[5].z;

	return p;
}

Pyramid Hexahedron::getPyramid2() const
{
	Pyramid p;

	p.v[0].x = v[3].x;
	p.v[0].y = v[3].y;
	p.v[0].z = v[3].z;

	p.v[1].x = v[0].x;
	p.v[1].y = v[0].y;
	p.v[1].z = v[0].z;

	p.v[2].x = v[1].x;
	p.v[2].y = v[1].y;
	p.v[2].z = v[1].z;

	p.v[3].x = v[5].x;
	p.v[3].y = v[5].y;
	p.v[3].z = v[5].z;

	p.v[4].x = v[4].x;
	p.v[4].y = v[4].y;
	p.v[4].z = v[4].z;

	return p;
}

double Hexahedron::getVolume() const
{
	return getPyramid0().getVolume() +
		   getPyramid1().getVolume() +
		   getPyramid2().getVolume();
}
