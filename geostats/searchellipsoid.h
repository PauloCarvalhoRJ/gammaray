#ifndef SEARCHELLIPSOID_H
#define SEARCHELLIPSOID_H

#include "searchneighborhood.h"

/** This class represents a search neighborhood in the shape of an ellipsoid.
 *   TODO: add support for rotation angles (azimuth, dip and roll).
 */
class SearchEllipsoid : public SearchNeighborhood
{
public:
	SearchEllipsoid(double hMax, double hMin, double hVert);

	/** Move constructor. */
	SearchEllipsoid( SearchEllipsoid&& right_hand_side );


//SearchNeighborhood interface
	virtual void getBBox( double centerX, double centerY, double centerZ,
						  double& minX, double& minY, double& minZ,
						  double& maxX, double& maxY, double& maxZ ) const;
	virtual bool isInside(double centerX, double centerY, double centerZ,
						  double x, double y, double z ) const;


	double m_hMax, m_hMin, m_hVert;
};

#endif // SEARCHELLIPSOID_H
