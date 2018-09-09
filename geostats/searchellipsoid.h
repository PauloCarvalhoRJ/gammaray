#ifndef SEARCHELLIPSOID_H
#define SEARCHELLIPSOID_H

#include "searchneighborhood.h"
#include "geostatsutils.h"

/** This class represents a search neighborhood in the shape of an ellipsoid.
 */
class SearchEllipsoid : public SearchNeighborhood
{
public:
    SearchEllipsoid(double hMax, double hMin, double hVert,
                    double azimuth, double dip, double roll,
                    uint numberOfSectors, uint minSamplesPerSector, uint maxSamplesPerSector);

	/** Move constructor. */
	SearchEllipsoid( SearchEllipsoid&& right_hand_side );


//SearchNeighborhood interface
	virtual void getBBox( double centerX, double centerY, double centerZ,
						  double& minX, double& minY, double& minZ,
						  double& maxX, double& maxY, double& maxZ ) const;
	virtual bool isInside(double centerX, double centerY, double centerZ,
						  double x, double y, double z ) const;
    virtual bool hasSpatialFiltering() const { return true; }
    virtual void performSpatialFilter( std::vector< SpatialLocationPtr >& samplesLocations ) const;


	double m_hMax, m_hMin, m_hVert;
	double m_azimuth, m_dip, m_roll;
    uint m_numberOfSectors, m_minSamplesPerSector, m_maxSamplesPerSector; //if m_numberOfSectors == 1, then sector setting is not used or effective

protected:
	Matrix3X3<double> m_rotationTransform;

	void setRotationTransform();
};

#endif // SEARCHELLIPSOID_H
