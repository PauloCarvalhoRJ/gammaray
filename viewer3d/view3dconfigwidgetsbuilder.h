#ifndef VIEW3DCONFIGWIDGETSBUILDER_H
#define VIEW3DCONFIGWIDGETSBUILDER_H

#include "view3dviewdata.h"

class View3DConfigWidget;
class ProjectComponent;
class Attribute;
class CartesianGrid;
class GeoGrid;
class SegmentSet;
class PointSet;

/**
 * This class groups static functions to build 3D viewer configuration widgets for the several domain objects.
 * In other words, if you need visualization configurations for an object visible in the 3D Viewer, then you need
 * to create a build() function for the object's class.  You also need to implement the
 * ProjectComponent::build3DViewerConfigWidget() in the target domain class.
 */
class View3DConfigWidgetsBuilder
{
public:
    View3DConfigWidgetsBuilder();

    /** Generic builder (fallback implementation). */
    static View3DConfigWidget* build( ProjectComponent *pc, View3DViewData viewObjects );

    //@{
    /** Specific overrides. */
    static View3DConfigWidget* build( Attribute *attribute, View3DViewData viewObjects ); //attribute (can be of point set, cartesian grid, etc.)
    //@}

private:
    /** Specific builder for an Attribute in a generic 3D Cartesian grid.*/
    static View3DConfigWidget* buildForAttribute3DCartesianGrid(
            CartesianGrid* cartesianGrid,
            Attribute* attribute ,
            View3DViewData viewObjects);

    /** Specific builder for an Attribute in a 2D map (nK < 2) Cartesian grid.
     * rendered as a flat surface.
     */
    static View3DConfigWidget* buildForAttributeMapCartesianGrid(
            CartesianGrid* cartesianGrid,
            Attribute* attribute ,
            View3DViewData viewObjects);

    /** Specific builder for an Attribute in a 2D map (nK < 2) Cartesian grid
     * rendered as contour lines.
     */
    static View3DConfigWidget* buildForAttributeContourLines(
            CartesianGrid* cartesianGrid,
            Attribute* attribute ,
            View3DViewData viewObjects);

    /** Specific builder for an Attribute in a GeoGrid.*/
    static View3DConfigWidget* buildForAttributeGeoGrid(
            GeoGrid* geoGrid,
            Attribute* attribute ,
            View3DViewData viewObjects);

    /** Specific builder for an Attribute in a SegmentSet.*/
    static View3DConfigWidget* buildForAttributeInSegmentSet(
            SegmentSet* segmentSet,
            Attribute* attribute,
            View3DViewData viewObjects );

    /** Specific builder for an Attribute in a PointSet.*/
    static View3DConfigWidget* buildForAttributeInPointSet(
            PointSet* pointSet,
            Attribute* attribute,
            View3DViewData viewObjects );
};

#endif // VIEW3DCONFIGWIDGETSBUILDER_H
