#ifndef SEARCHANNULUSSTRATIGRAPHIC_H
#define SEARCHANNULUSSTRATIGRAPHIC_H

#include "geostats/searchannulus.h"

class GeoGrid;

/**
 * The SearchAnnulusStratigraphic class represents a search neighborhood with the shape of an annulus (2D ring)
 * along a given stratigraphic interval (topological index K) of a geological grid (class GeoGrid).
 * Being 2D, this search neighborhood is recommended for usage with 2D data sets.
 */
class SearchAnnulusStratigraphic : public SearchAnnulus
{
public:
    SearchAnnulusStratigraphic( double innerRadius, double outerRadius, GeoGrid* geoGrid );

    /** Move constructor. */
    SearchAnnulusStratigraphic( SearchAnnulusStratigraphic&& right_hand_side );


    ////-------------SearchNeighborhood interface------------------------------------------
    /**
     * @note As this search neighborhood is 2D, the z coordinates are ignored in the test.
     */
    virtual bool isInside(double centerX, double centerY, double centerZ,
                          double x, double y, double z ) const;
    ////-------------------------------------------------------------------------------------


    GeoGrid* m_geoGrid;
};

#endif // SEARCHANNULUSSTRATIGRAPHIC_H
