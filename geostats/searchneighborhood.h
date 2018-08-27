#ifndef SEARCHNEIGHBORHOOD_H
#define SEARCHNEIGHBORHOOD_H

#include <memory>

/** This class represents a generic search neighborhood. */
class SearchNeighborhood
{
public:
	SearchNeighborhood();

	/** Computes the bounding box of this neighborhood when centered at the given location. */
	virtual void getBBox( double centerX, double centerY, double centerZ,
						  double& minX, double& minY, double& minZ,
                          double& maxX, double& maxY, double& maxZ ) const = 0;

	/** Tests whether the given point is inside the neighborhood centered at (centerX, centerY, centerZ). */
	virtual bool isInside(double centerX, double centerY, double centerZ,
						  double x, double y, double z ) const = 0;
};

typedef std::shared_ptr<SearchNeighborhood> SearchNeighborhoodPtr;

#endif // SEARCHNEIGHBORHOOD_H
