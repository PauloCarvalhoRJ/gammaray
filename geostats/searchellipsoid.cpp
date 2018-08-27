#include "searchellipsoid.h"
#include <utility>
#include <iostream>

SearchEllipsoid::SearchEllipsoid(double hMax, double hMin, double hVert) :
	m_hMax( hMax ), m_hMin( hMin ), m_hVert( hVert )
{
}

SearchEllipsoid::SearchEllipsoid(SearchEllipsoid && right_hand_side) :
	SearchNeighborhood( std::move( right_hand_side ) ), //moves the base part of the object
	m_hMax( right_hand_side.m_hMax ),
	m_hMin( right_hand_side.m_hMin ),
	m_hVert( right_hand_side.m_hVert )
{
}

void SearchEllipsoid::getBBox(double centerX, double centerY, double centerZ,
							  double & minX, double & minY, double & minZ,
							  double & maxX, double & maxY, double & maxZ) const
{
    //Without azimuth, it is assumed zero.  By convention zero azimuth is north, this hMax points to north (Y axis).
	maxY = centerY + m_hMax;
	minY = centerY - m_hMax;
	maxX = centerX + m_hMin;
	minX = centerX - m_hMin;
	maxZ = centerZ + m_hVert;
	minZ = centerZ - m_hVert;
}

bool SearchEllipsoid::isInside(double centerX, double centerY, double centerZ,
							   double x, double y, double z) const
{
	//Without azimuth, it is assumed zero.  By convention zero azimuth is north, this hMax points to north (Y axis).
	double dx = x - centerX;
	double dy = y - centerY;
	double dz = z - centerZ;
	return dx*dx/m_hMin + dy*dy/m_hMax + dz*dz/m_hVert <= 1.0;
}
