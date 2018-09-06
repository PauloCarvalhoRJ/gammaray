#include "searchellipsoid.h"
#include <utility>
#include <iostream>

SearchEllipsoid::SearchEllipsoid( double hMax, double hMin, double hVert,
								  double azimuth, double dip, double roll ) :
	m_hMax( hMax ), m_hMin( hMin ), m_hVert( hVert ),
	m_azimuth( azimuth ), m_dip( dip ), m_roll( roll )
{
	setRotationTransform();
}

SearchEllipsoid::SearchEllipsoid(SearchEllipsoid && right_hand_side) :
	SearchNeighborhood( std::move( right_hand_side ) ), //moves the base part of the object
	m_hMax( right_hand_side.m_hMax ),
	m_hMin( right_hand_side.m_hMin ),
	m_hVert( right_hand_side.m_hVert ),
	m_azimuth( right_hand_side.m_azimuth ),
	m_dip( right_hand_side.m_dip ),
	m_roll( right_hand_side.m_roll )
{
	setRotationTransform();
}

void SearchEllipsoid::getBBox( double centerX, double centerY, double centerZ,
							   double & minX, double & minY, double & minZ,
							   double & maxX, double & maxY, double & maxZ ) const
{
	TODO_COMPUTE_BUNDINGBOX_GENERIC_ELLIPSOID;

    //Without azimuth, it is assumed zero.  By convention zero azimuth is north, this hMax points to north (Y axis).
	maxY = centerY + m_hMax;
	minY = centerY - m_hMax;
	maxX = centerX + m_hMin;
	minX = centerX - m_hMin;
	maxZ = centerZ + m_hVert;
	minZ = centerZ - m_hVert;
}

bool SearchEllipsoid::isInside( double centerX, double centerY, double centerZ,
								double x,       double y,       double z        ) const
{
	double localX = x - centerX;
	double localY = y - centerY;
	double localZ = z - centerZ;
	GeostatsUtils::transform( m_rotationTransform, localX, localY, localZ );
	//Without azimuth, it is assumed zero.  By convention zero azimuth is north, this hMax points to north (Y axis).
	double dx = localX/m_hMin;
	double dy = localY/m_hMax;
	double dz = localZ/m_hVert;
	return dx*dx + dy*dy + dz*dz <= 1.0;
}

void SearchEllipsoid::setRotationTransform()
{
	m_rotationTransform = GeostatsUtils::getAnisoTransform( 1.0, 1.0, 1.0, m_azimuth, m_dip, m_roll );
}
