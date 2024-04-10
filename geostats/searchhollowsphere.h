#ifndef SEARCHHOLLOWSPHERE_H
#define SEARCHHOLLOWSPHERE_H

#include "geostats/searchannulus.h"

/**
 * The SearchHollowSphere class represents a search neighborhood with the shape of a hollow sphere.
 * It works like the SearchAnnulus for 3D datasets.
 */
class SearchHollowSphere : public SearchAnnulus
{
public:
    SearchHollowSphere( double innerRadius, double outerRadius );

    /** Move constructor. */
    SearchHollowSphere( SearchHollowSphere&& right_hand_side );


    ////-------------SearchNeighborhood interface------------------------------------------
    virtual void getBBox( double centerX, double centerY, double centerZ,
                          double& minX, double& minY, double& minZ,
                          double& maxX, double& maxY, double& maxZ ) const;

    virtual bool isInside(double centerX, double centerY, double centerZ,
                          double x, double y, double z ) const;
    ////-------------------------------------------------------------------------------------
};

#endif // SEARCHHOLLOWSPHERE_H
