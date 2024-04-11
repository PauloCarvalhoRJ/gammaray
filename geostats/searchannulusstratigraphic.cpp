#include "searchannulusstratigraphic.h"

#include "domain/geogrid.h"

SearchAnnulusStratigraphic::SearchAnnulusStratigraphic(double innerRadius, double outerRadius, GeoGrid* geoGrid) :
    SearchAnnulus( innerRadius, outerRadius ),
    m_geoGrid( geoGrid )
{}

bool SearchAnnulusStratigraphic::isInside(double centerX, double centerY, double centerZ,
                                          double x, double y, double z) const
{
    //determine the stratigraphic intervals corresponding to the
    //reference location and to the query location
    uint I, J, refK, queryK;
    m_geoGrid->XYZtoIJK(centerX, centerY, centerZ, I, J, refK);
    m_geoGrid->XYZtoIJK(x, y, z, I, J, queryK);

    //if both locations don't lie in the same stratigraphic interval, return a miss.
    if( refK != queryK )
        return false;

    //if the locations belong to the same stratigraphic interval, do the same geometric in/out
    //test as in SeachAnnulus for 2D datasets.
    return SearchAnnulus::isInside( centerX, centerY, centerZ, x, y, z );
}
