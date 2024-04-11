#ifndef SEARCHVERTICALDUMBBELL_H
#define SEARCHVERTICALDUMBBELL_H

#include "geostats/searchneighborhood.h"

/**
 * The SearchVerticalDumbbell class represents a search neighborhood with the shape of a vertical dumbbell,
 * that is: two vertical cylinders arranged along the vertical line with a separation between them but without an axle.
 * This search neighborhood is recommended for searching samples along the vertical in 3D datasets.
 */
class SearchVerticalDumbbell : public SearchNeighborhood
{
public:
    SearchVerticalDumbbell(double height_of_each_cylinder,
                           double separation_between_the_cylinders,
                           double radius);


    /** Move constructor. */
    SearchVerticalDumbbell( SearchVerticalDumbbell&& right_hand_side );


    ////-------------SearchNeighborhood interface------------------------------------------
    virtual void getBBox( double centerX, double centerY, double centerZ,
                          double& minX, double& minY, double& minZ,
                          double& maxX, double& maxY, double& maxZ ) const;

    virtual bool isInside(double centerX, double centerY, double centerZ,
                          double x, double y, double z ) const;

    virtual bool hasSpatialFiltering() const { return false; }

    virtual void performSpatialFilter( double centerX, double centerY, double centerZ,
                                       std::vector< IndexedSpatialLocationPtr >& samplesLocations,
                                       const SearchStrategy& parentSearchStrategy ) const {}
    ////-------------------------------------------------------------------------------------

    double m_height_of_each_cylinder;
    double m_separation_between_the_cylinders;
    double m_radius;
};

#endif // SEARCHVERTICALDUMBBELL_H
