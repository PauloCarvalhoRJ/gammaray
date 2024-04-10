#ifndef SEARCHWASHER_H
#define SEARCHWASHER_H

#include "geostats/searchannulus.h"

/**
 * The SearchWasher class represents a search neighborhood with the shape of a washer.
 * The shape extends that of the SearchAnnulus to include a [normally small] thickness in Z direction
 * so it can be used for lateral searches in 3D datasets like that of the SearchAnnulus in 2D datasets.
 */
class SearchWasher : public SearchAnnulus
{
public:
    SearchWasher(double innerRadius, double outerRadius, double zThickness);

    /** Move constructor. */
    SearchWasher( SearchWasher&& right_hand_side );

    ////-------------SearchNeighborhood interface------------------------------------------
    virtual void getBBox( double centerX, double centerY, double centerZ,
                          double& minX, double& minY, double& minZ,
                          double& maxX, double& maxY, double& maxZ ) const;

    virtual bool isInside(double centerX, double centerY, double centerZ,
                          double x, double y, double z ) const;
    ////-------------------------------------------------------------------------------------

    double m_zThickness;
};

#endif // SEARCHWASHER_H
