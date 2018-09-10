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


////-------------SearchNeighborhood interface------------------------------------------
	virtual void getBBox( double centerX, double centerY, double centerZ,
						  double& minX, double& minY, double& minZ,
						  double& maxX, double& maxY, double& maxZ ) const;
	virtual bool isInside(double centerX, double centerY, double centerZ,
						  double x, double y, double z ) const;

	/** If the user set just one sector (entire azimuth span) then effectivelly there is no filtering. */
	virtual bool hasSpatialFiltering() const { return m_numberOfSectors > 1; }

	/** The search ellipsoid's spatial filter divides the azimuth range into a number of sectors (given by m_numberOfSectors),
	 * then the locations are put into azimuth bins (each corresponding to a sector).  If the minimum number of samples (given by
	 * m_minSamplesPerSector) is not reached in one of the bins, the list is emptied (search fails).  If a bin has more samples
	 * than allowed (given by m_maxSamplesPerSector) then the most distant samples are removed from the respective bin so this
	 * maximum remains.  Partition only occurs in XY plane, thus the Z coordinate is ignored.
	 */
	virtual void performSpatialFilter( double centerX, double centerY, double centerZ,
									   std::vector< IndexedSpatialLocationPtr >& samplesLocations,
									   const SearchStrategy& parentSearchStrategy ) const;
////-------------------------------------------------------------------------------------


	double m_hMax, m_hMin, m_hVert;
	double m_azimuth, m_dip, m_roll;
    uint m_numberOfSectors, m_minSamplesPerSector, m_maxSamplesPerSector; //if m_numberOfSectors == 1, then sector setting is not used or effective

protected:
	Matrix3X3<double> m_rotationTransform;

	void setRotationTransform();
};

#endif // SEARCHELLIPSOID_H
