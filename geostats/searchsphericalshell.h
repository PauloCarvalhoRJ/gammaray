#ifndef SEARCHSPHERICALSHELL_H
#define SEARCHSPHERICALSHELL_H

#include "geostats/searchannulus.h"

/**
 * The SearchSphericalShell class represents a search neighborhood with the shape of a hollow sphere.
 * It works like the SearchAnnulus for 3D datasets.
 */
class SearchSphericalShell : public SearchAnnulus
{
public:
    SearchSphericalShell( double innerRadius, double outerRadius );

    /** Move constructor. */
    SearchSphericalShell( SearchSphericalShell&& right_hand_side );


    ////-------------SearchNeighborhood interface------------------------------------------
    virtual void getBBox( double centerX, double centerY, double centerZ,
                          double& minX, double& minY, double& minZ,
                          double& maxX, double& maxY, double& maxZ ) const;

    virtual bool isInside(double centerX, double centerY, double centerZ,
                          double x, double y, double z ) const;
    ////-------------------------------------------------------------------------------------
};

#endif // SEARCHSPHERICALSHELL_H
